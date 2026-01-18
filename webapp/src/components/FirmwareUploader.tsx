import React, { useState, useEffect, useRef } from "react";
import {
  Typography,
  Button,
  Box,
  LinearProgress,
  Alert,
  Card,
  CardContent,
  Chip,
  Grid,
  Checkbox,
  FormControlLabel,
  Accordion,
  AccordionSummary,
  AccordionDetails,
  Stack,
  Link,
} from "@mui/material";
import {
  Upload as UploadIcon,
  UsbOff as UsbOffIcon,
  FlashOn as FlashOnIcon,
  ExpandMore as ExpandMoreIcon,
} from "@mui/icons-material";
import { useSerialStore } from "../stores/serial";
import Console from "./SerialConsole";

// type UploadStep = "select" | "bootloader" | "flash";

const FirmwareUploader: React.FC = () => {
  const {
    chipInfo,
    isFlashing,
    flashProgress,
    log,
    isConnected,
    flashFirmware,
    disconnect,
    init,
    availableFirmware,
  } = useSerialStore();

  // const [step, setStep] = useState<UploadStep>("select");
  const [file, setFile] = useState<File | null>(null);
  const [selectedFirmware, setSelectedFirmware] = useState("");

  // Whether manual bootloader mode is required (default depends on board)
  const [bootloaderRequired, setBootloaderRequired] = useState(true);
  const [bootConfirmed, setBootConfirmed] = useState(false);

  const [fwOpen, setFwOpen] = useState(true);
  const [bootOpen, setBootOpen] = useState(false);
  const [flashOpen, setFlashOpen] = useState(false);
  const [manualOpen, setManualOpen] = useState(false);

  const logEndRef = useRef<HTMLDivElement>(null);

  useEffect(() => init(), [init]);
  useEffect(
    () => logEndRef.current?.scrollIntoView({ behavior: "smooth" }),
    [log],
  );

  // const resetUploader = (): void => {
  //   setStep("select");
  //   setFile(null);
  //   setSelectedFirmware("");
  //   setBootloaderRequired(false);
  //   setBootConfirmed(false);

  //   setFwOpen(true);
  //   setBootOpen(false);
  //   setFlashOpen(false);
  // };

  const handleFileChange = (e: React.ChangeEvent<HTMLInputElement>): void => {
    const f = e.target.files?.[0];
    if (!f || !f.name.endsWith(".bin")) return;
    setFile(f);

    // Step 2 opens if bootloader required, otherwise flash directly
    setBootOpen(bootloaderRequired);
    setFlashOpen(!bootloaderRequired);
    // setStep("bootloader");
  };

  const handlePresetSelect = async (fw: any): Promise<void> => {
    setSelectedFirmware(fw.label);

    // Auto-set bootloader requirement based on board
    const requiresBootloader = fw.board === "LOLIN S2 Mini";
    setBootloaderRequired(requiresBootloader);

    try {
      const res = await fetch(fw.path);
      if (!res.ok) throw new Error(res.statusText);

      const blob = await res.blob();
      const f = new File([blob], fw.path.split("/").pop()!, {
        type: "application/octet-stream",
      });

      setFile(f);
      setFwOpen(false);

      setBootOpen(requiresBootloader);
      setFlashOpen(!requiresBootloader);
      // setStep("bootloader");
    } catch {
      alert("Failed to load firmware.");
    }
  };

  const handleConnectAndFlash = async (): Promise<void> => {
    if (!file || (bootloaderRequired && !bootConfirmed)) return;

    try {
      // Pass bootloaderRequired to the store for flashing logic
      await flashFirmware(file, bootloaderRequired);
    } catch (err) {
      console.error(err);
    }
  };

  return (
    <Box p={2}>
      <Box mb={2}>
        <Alert severity="info">
          Please note, this firmware uploader is an experimental feature. If you
          encounter any issues, please report them on our <Link href="https://github.com/thomasgeissl/NeopixelCommander" target="_blank" rel="noopener noreferrer">GitHub issues page</Link>.{" "}
          <br />
          You can also use the Arduino IDE to flash firmware manually if you
          prefer. Use the library manager to install the NeopixelCommander
          library and select the appropriate example sketch for your board.
          <br />
          <Button variant="outlined" onClick={() => setManualOpen(true)}>
            Show instructions
          </Button>
        </Alert>
      </Box>
      <Grid container spacing={2}>
        {/* Step 1: Firmware selection */}
        <Grid size={{ xs: 12 }}>
          <Accordion
            expanded={fwOpen}
            disabled={isFlashing}
            onChange={(_, expanded) => setFwOpen(expanded)}
          >
            <AccordionSummary expandIcon={<ExpandMoreIcon />}>
              <Stack
                direction="row"
                spacing={1}
                alignItems="center"
                flexWrap="wrap"
              >
                <Typography variant="subtitle2">
                  Step 1 — Select firmware
                </Typography>
                {file && (
                  <>
                    {selectedFirmware && (
                      <Chip
                        size="small"
                        color="primary"
                        label={selectedFirmware}
                      />
                    )}
                    <Chip
                      size="small"
                      color="success"
                      label={`${(file.size / 1024).toFixed(2)} KB`}
                    />
                  </>
                )}
              </Stack>
            </AccordionSummary>

            <AccordionDetails>
              <Box
                sx={{ display: "flex", flexDirection: "column", gap: 2, mb: 2 }}
              >
                {availableFirmware.map((fw) => (
                  <Button
                    key={fw.label}
                    variant={
                      selectedFirmware === fw.label ? "contained" : "outlined"
                    }
                    size="large"
                    fullWidth
                    onClick={() => handlePresetSelect(fw)}
                    disabled={isFlashing}
                    sx={{
                      py: 2,
                      justifyContent: "flex-start",
                      textAlign: "left",
                    }}
                  >
                    <Box>
                      <Typography
                        variant="body1"
                        sx={{ fontWeight: 600, lineHeight: 1.2 }}
                      >
                        {fw.label}
                      </Typography>
                      <Typography
                        variant="caption"
                        color="text.secondary"
                        sx={{ display: "block", lineHeight: 1.3 }}
                      >
                        {fw.board} — {fw.description}
                      </Typography>
                    </Box>
                  </Button>
                ))}
              </Box>

              <Button
                variant="outlined"
                component="label"
                fullWidth
                size="large"
                startIcon={<UploadIcon />}
                sx={{ borderStyle: "dashed", borderWidth: 2 }}
                disabled={isFlashing}
              >
                {file ? file.name : "Or select custom .bin file"}
                <input
                  type="file"
                  hidden
                  accept=".bin"
                  onChange={handleFileChange}
                />
              </Button>
            </AccordionDetails>
          </Accordion>
        </Grid>

        {/* Step 2: Bootloader toggle + confirmation */}
        <Grid size={{ xs: 12 }}>
          <Accordion
            expanded={bootOpen}
            disabled={!file || isFlashing}
            onChange={(_, expanded) => setBootOpen(expanded)}
          >
            <AccordionSummary expandIcon={<ExpandMoreIcon />}>
              <Stack direction="row" spacing={1} alignItems="center">
                <Typography variant="subtitle2">
                  Step 2 — Bootloader mode
                </Typography>
                {bootConfirmed && (
                  <Chip size="small" color="success" label="Confirmed" />
                )}
              </Stack>
            </AccordionSummary>

            <AccordionDetails>
              {/* Toggle bootloader requirement */}
              <FormControlLabel
                control={
                  <Checkbox
                    checked={bootloaderRequired}
                    onChange={(e) => setBootloaderRequired(e.target.checked)}
                  />
                }
                label="Manual bootloader required (hold BOOT + RESET)"
              />
              <Typography
                variant="caption"
                color="text.secondary"
                sx={{ display: "block", mb: 2, ml: 3 }}
              >
                Some boards, like LOLIN S2 Mini, must be manually set into
                bootloader mode by holding BOOT and pressing RESET. This is
                required for flashing and is not related to any software
                settings.
              </Typography>

              {/* Only show confirmation if bootloader is required */}
              {bootloaderRequired && (
                <>
                  <Alert severity="warning" sx={{ my: 2 }}>
                    Place the device in manual bootloader mode: hold BOOT and
                    press RESET.
                  </Alert>

                  <FormControlLabel
                    control={
                      <Checkbox
                        checked={bootConfirmed}
                        onChange={(e) => {
                          const checked = e.target.checked;
                          setBootConfirmed(checked);

                          if (checked) {
                            setBootOpen(false);
                            setFlashOpen(true);
                            // setStep("flash");
                          } else {
                            setFlashOpen(false);
                            // setStep("bootloader");
                          }
                        }}
                      />
                    }
                    label="I have placed the device in bootloader mode"
                  />
                </>
              )}
            </AccordionDetails>
          </Accordion>
        </Grid>

        {/* Step 3: Flash */}
        <Grid size={{ xs: 12 }}>
          <Accordion
            expanded={flashOpen}
            disabled={bootloaderRequired && !bootConfirmed}
          >
            <AccordionSummary expandIcon={<ExpandMoreIcon />}>
              <Stack direction="row" spacing={1} alignItems="center">
                <Typography variant="subtitle2">
                  Step 3 — Flash firmware
                </Typography>
                {chipInfo && (
                  <Chip size="small" color="primary" label={chipInfo} />
                )}
                {isConnected && !isFlashing && (
                  <Chip size="small" color="success" label="Connected" />
                )}
                {isFlashing && (
                  <Chip size="small" color="warning" label="Flashing" />
                )}
              </Stack>
            </AccordionSummary>

            <AccordionDetails>
              <Button
                fullWidth
                size="large"
                variant="contained"
                startIcon={<FlashOnIcon />}
                onClick={handleConnectAndFlash}
                disabled={(bootloaderRequired && !bootConfirmed) || isFlashing}
              >
                {isFlashing ? "Flashing…" : "Connect & Flash"}
              </Button>

              {isConnected && !isFlashing && (
                <Button
                  sx={{ mt: 1 }}
                  fullWidth
                  variant="outlined"
                  color="error"
                  startIcon={<UsbOffIcon />}
                  onClick={disconnect}
                >
                  Disconnect
                </Button>
              )}
            </AccordionDetails>
          </Accordion>
        </Grid>
        <Grid size={{ xs: 12 }}>
          <Accordion
            expanded={manualOpen}
            onChange={(_, expanded) => setManualOpen(expanded)}
            sx={{ opacity: 0.85 }}
          >
            <AccordionSummary expandIcon={<ExpandMoreIcon />}>
              <Typography variant="caption" color="text.secondary">
                Alternative: manual upload using Arduino IDE
              </Typography>
            </AccordionSummary>

            <AccordionDetails>
              <Typography variant="body2" sx={{ mb: 1 }}>
                Firmware may also be uploaded manually using the Arduino IDE.
                This path exists as a fallback and is not required for normal
                operation.
              </Typography>

              <Typography variant="body2" component="ul" sx={{ pl: 2 }}>
                <li>Install the Arduino IDE.</li>
                <li>
                  Install the appropriate board package via Boards Manager, e.g
                  esp32 by espressif systems.
                </li>
                <li>
                  Install the NeopixelCommander library via Library Manager.
                </li>
                <li>Choose one of the example sketches from the library</li>
                <li>Select the correct board and upload the example sketch.</li>
              </Typography>

              <Typography
                variant="caption"
                color="text.secondary"
                sx={{ mt: 1 }}
              >
                Some boards require manual bootloader mode during upload. E.g. for a LOLIN S2 Mini hold BOOT and press RESET to enter bootloader mode.
              </Typography>
            </AccordionDetails>
          </Accordion>
        </Grid>
      </Grid>

      {flashProgress && flashProgress.total > 0 && (
        <Card sx={{ mt: 3 }}>
          <CardContent>
            <Typography variant="body2">
              {flashProgress.percentage.toFixed(1)}%
            </Typography>
            <LinearProgress
              variant="determinate"
              value={flashProgress.percentage}
            />
          </CardContent>
        </Card>
      )}

      <Console canSend={false} height={200} />
      <div ref={logEndRef} />
    </Box>
  );
};

export default FirmwareUploader;
