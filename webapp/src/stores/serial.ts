import { create } from "zustand";
import { devtools } from "zustand/middleware";
import { ESPLoader, Transport } from "esptool-js";
import CryptoJS from "crypto-js";
// import axios from "axios";

// https://github.com/adafruit/Adafruit_WebSerial_ESPTool/blob/main/js/script.js

/* ----------------------------- Types ----------------------------- */

type LogType = "send" | "receive" | "error" | "system";

interface LogEntry {
  message: string;
  type: LogType;
  timestamp: Date;
}

interface FlashProgress {
  written: number;
  total: number;
  percentage: number;
}

interface SerialState {
  port: SerialPort | null;
  reader: ReadableStreamDefaultReader<string> | null;
  writer: WritableStreamDefaultWriter<string> | null;
  readerAbortController: AbortController | null;
  writerAbortController: AbortController | null;

  isConnected: boolean;
  isMonitoring: boolean;
  isFlashing: boolean;

  log: LogEntry[];
  flashProgress: FlashProgress | null;
  chipInfo: string;
  availableFirmware: any[];

  init: () => void;

  connect(): Promise<void>;
  disconnect(): Promise<void>;

  startMonitoring(): Promise<void>;
  stopMonitoring(): Promise<void>;

  send(data: string): Promise<void>;
  clearLog(): void;

  flashFirmware(
    file: File,
    manualBootloaderRequired: boolean,
    address?: number
  ): Promise<void>;
}

/* ---------------------------- Helpers ---------------------------- */

function uint8ArrayToBinaryString(arr: Uint8Array): string {
  const chunkSize = 0x8000;
  let result = "";
  for (let i = 0; i < arr.length; i += chunkSize) {
    result += String.fromCharCode(...arr.subarray(i, i + chunkSize));
  }
  return result;
}

const terminal = {
  clean() {},
  writeLine(t: string) {
    console.log("[esptool]", t);
  },
  write(t: string) {
    console.log("[esptool]", t);
  },
};

/* ---------------------------- Store ----------------------------- */

export const useSerialStore = create<SerialState>()(
  devtools((set, get) => {
    let readLoopActive = false;

    const addLog = (message: string, type: LogType) => {
      set((s) => ({
        log: [...s.log, { message, type, timestamp: new Date() }],
      }));
    };

    /* ------------------------ Monitoring ------------------------ */

    const startReadLoop = async (
      reader: ReadableStreamDefaultReader<string>
    ) => {
      readLoopActive = true;
      try {
        while (readLoopActive) {
          const { value, done } = await reader.read();
          if (done) break;
          if (value) addLog(value, "receive");
        }
      } catch {}
    };

    const startMonitoring = async () => {
      const { port, isFlashing } = get();
      if (!port || isFlashing) return;

      const decoder = new TextDecoderStream();
      const encoder = new TextEncoderStream();

      // Store abort controllers to cancel pipes later
      const readerAbortController = new AbortController();
      const writerAbortController = new AbortController();

      // Catch abort errors from pipes (they're expected when we disconnect)
      port.readable!.pipeTo(decoder.writable as WritableStream<Uint8Array>, {
        signal: readerAbortController.signal
      }).catch(() => {
        // Expected abort error when disconnecting, ignore
      });
      
      encoder.readable.pipeTo(port.writable!, {
        signal: writerAbortController.signal
      }).catch(() => {
        // Expected abort error when disconnecting, ignore
      });

      const reader = decoder.readable.getReader();
      const writer = encoder.writable.getWriter();

      set({ 
        reader, 
        writer, 
        isMonitoring: true,
        readerAbortController,
        writerAbortController
      });
      startReadLoop(reader);
    };

    const stopMonitoring = async () => {
      readLoopActive = false;
      const { reader, writer, readerAbortController, writerAbortController } = get();
      
      // Abort the pipes first (this will throw AbortErrors, which is expected)
      try {
        readerAbortController?.abort();
      } catch (e) {
        // Expected abort error, ignore
      }
      
      try {
        writerAbortController?.abort();
      } catch (e) {
        // Expected abort error, ignore
      }
      
      // Then release locks
      try {
        if (reader) {
          reader.releaseLock();
        }
      } catch (e) {
        console.error('Error releasing reader:', e);
      }
      
      try {
        if (writer) {
          writer.releaseLock();
        }
      } catch (e) {
        console.error('Error releasing writer:', e);
      }
      
      // Small delay to ensure streams are fully released
      await new Promise(resolve => setTimeout(resolve, 100));
      
      set({ 
        reader: null, 
        writer: null, 
        isMonitoring: false,
        readerAbortController: null,
        writerAbortController: null
      });
    };

    /* ------------------------- Public API ------------------------- */

    return {
      port: null,
      reader: null,
      writer: null,
      readerAbortController: null,
      writerAbortController: null,

      isConnected: false,
      isMonitoring: false,
      isFlashing: false,

      log: [],
      flashProgress: null,
      chipInfo: "",
      availableFirmware: [
        {
          label: "NeoPixel Commander Server",
          path: "/NeopixelCommander/firmware/lolin_s2_mini_example.ino.bin",
          description: "Firmware to be controlled via rest and websocket API",
          board: "LOLIN S2 Mini",
        },
      ],

      init: () => {
        // axios.get("https://api.github.com/repos/grantler-instruments/esp-now-midi/releases/latest")
        //   .then((response) => {
        //     const { tag_name, assets } = response.data;
        //     console.log("Latest firmware version:", tag_name);
        //     const firmwareAssets = assets.filter((asset: any) =>
        //       asset.name.endsWith(".bin") || asset.name.endsWith(".bin.gz")
        //     )
        //     .filter((asset: any) => !(asset.name.includes("merged") || asset.name.includes("partition") || asset.name.includes("bootloader")));
        //     console.log("Available firmware assets:", firmwareAssets);
        //     set({ availableFirmware: firmwareAssets });
        //   })
        //   .catch((error) => {
        //     console.error("Error fetching firmware info:", error);
        //   });
      },
      
      connect: async () => {
        if (get().isConnected || get().isFlashing) return;

        const port = await navigator.serial.requestPort();
        await port.open({ baudRate: 115200 });

        set({ port, isConnected: true });
        addLog("Serial port opened", "system");
        await startMonitoring();
      },

      disconnect: async () => {
        if (!get().port) return;
        await stopMonitoring();
        await get().port!.close();
        set({ port: null, isConnected: false });
        addLog("Disconnected", "system");
      },

      startMonitoring,
      stopMonitoring,

      send: async (data: string) => {
        if (!get().writer) return;
        await get().writer!.write(data + "\n");
        addLog(data, "send");
      },

      clearLog: () => set({ log: [] }),

      /* ------------------------- Flashing ------------------------- */

      flashFirmware: async (
        file: File,
        manualBootloaderRequired,
        address = 0x10000
      ) => {
        if (get().isFlashing) return;

        let port: SerialPort | null = null;
        let transport: Transport | null = null;

        try {
          addLog("Preparing for flashing…", "system");

          if (get().isMonitoring) await stopMonitoring();
          if (get().port) await get().port!.close();

          set({
            port: null,
            isConnected: false,
            isFlashing: true,
          });

          // Always request a *fresh, unopened* port
          port = await navigator.serial.requestPort();

          transport = new Transport(port, false);

          const loader = new ESPLoader({
            transport,
            baudrate: 115200,
            romBaudrate: 115200,
            terminal,
            debugLogging: false,
          });

          let chip: string;

          // try {
          //   await transport.connect(115200);
          //   chip = await loader.main("default_reset");
          // } catch {
          //   addLog("Retrying without reset signals…", "system");
          //   await transport.disconnect();
          //   await new Promise((r) => setTimeout(r, 300));
          //   await transport.connect(115200);
          //   chip = await loader.main("no_reset");
          // }

          chip = await loader.main(
            manualBootloaderRequired ? "no_reset" : "default_reset"
          );
          set({ chipInfo: chip });
          addLog(`Connected to ${chip}`, "system");

          const data = uint8ArrayToBinaryString(
            new Uint8Array(await file.arrayBuffer())
          );

          await loader.writeFlash({
            fileArray: [{ data, address }],
            flashSize: "keep",
            eraseAll: false,
            flashMode: "keep",
            flashFreq: "keep",
            reportProgress: (_, written, total) => {
              set({
                flashProgress: {
                  written,
                  total,
                  percentage: Math.floor((written / total) * 100),
                },
              });
            },
            compress: true,
            calculateMD5Hash: (i: string) =>
              CryptoJS.MD5(CryptoJS.enc.Latin1.parse(i)).toString(),
          });

          addLog("✓ Flash complete", "system");
          if (manualBootloaderRequired) {
            addLog(
              "Please press the reset button manually - sorry, was not yet able to automate this",
              "system"
            );
          }
        } finally {
          try {
            await transport?.disconnect();
            await transport?.waitForUnlock(1500);
          } catch {}
          try {
            await port?.close();
          } catch {}
          set({ isFlashing: false });
          addLog("Flash session closed.", "system");
        }
      },
    };
  })
);