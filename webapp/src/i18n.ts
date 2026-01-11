import i18n from "i18next";
import { initReactI18next } from "react-i18next";

i18n.use(initReactI18next).init({
  resources: {
    en: {
      translation: {
        "test": "This is a test string",
        "connect": "Connect",
        "disconnect": "Disconnect",
        "clear": "Clear",
        "made_with_love": "Made with ♡️ by",
        "tooltip_pin_to_midi": "Configure how input pins on the microcontroller map to MIDI messages.",
        "tooltip_midi_to_pin": "Set pins values from MIDI messages",
        "tooltip_wireless_midi": "Set up ESP-NOW MIDI connections to other devices",
        "tooltip_save_configuration_to_file": "Save current configuration to a file",
        "tooltip_load_configuration_from_file": "Load configuration from a file",
        "tooltip_load_configuration_from_device": "Load configuration from a device",
        "tooltip_midi_monitor": "View incoming and outgoing MIDI messages for debugging purposes",
        "tooltip_serial_monitor": "View serial output from the connected microcontroller for debugging purposes",
        "tooltip_midi_composer": "Compose and send custom MIDI messages to connected devices",
      },
    },
    de: {
      translation: {
        "connect": "Verbinden",
        "disconnect": "Trennen",
        "clear": "Löschen",
        "test": "Das ist ein Teststring",
        "made_with_love": "Entwickelt mit ♡️ von",
        "tooltip_pin_to_midi": "Konfiguriere, wie Eingangs-Pins auf dem Mikrocontroller MIDI-Nachrichten zugeordnet werden.",
        "tooltip_midi_to_pin": "Setze Pin-Werte von MIDI-Nachrichten",
        "tooltip_wireless_midi": "Richte ESP-NOW MIDI-Verbindungen zu anderen Geräten ein",
        "tooltip_save_configuration_to_file": "Aktuelle Konfiguration in einer Datei speichern",
        "tooltip_load_configuration_from_file": "Konfiguration aus einer Datei laden",
        "tooltip_load_configuration_from_device": "Konfiguration von einem Gerät laden",
        "tooltip_midi_monitor": "Eingehende und ausgehende MIDI-Nachrichten zu Debugging-Zwecken anzeigen",
        "tooltip_serial_monitor": "Serielle Ausgabe des verbundenen Mikrocontrollers zu Debugging-Zwecken anzeigen",
        "tooltip_midi_composer": "Erstelle und sende benutzerdefinierte MIDI-Nachrichten an verbundene Geräte",
      },
    },
    es: {
      translation: {
        "connect": "Conectar",
        "disconnect": "Desconectar",
        "clear": "Limpiar",
        "test": "Esta es una cadena de prueba",
        "made_with_love": "Desarrollado con ♡️ por",
        "tooltip_pin_to_midi": "Configurar cómo los pines de entrada en el microcontrolador se asignan a los mensajes MIDI.",
        "tooltip_midi_to_pin": "Establecer valores de pines desde mensajes MIDI",
        "tooltip_wireless_midi": "Configurar conexiones MIDI ESP-NOW a otros dispositivos",
        "tooltip_save_configuration_to_file": "Guardar la configuración actual en un archivo",
        "tooltip_load_configuration_from_file": "Cargar configuración desde un archivo",
        "tooltip_load_configuration_from_device": "Cargar configuración desde un dispositivo",
        "tooltip_midi_monitor": "Ver mensajes MIDI entrantes y salientes para fines de depuración",
        "tooltip_serial_monitor": "Ver la salida serial del microcontrolador conectado para fines de depuración",
        "tooltip_midi_composer": "Componer y enviar mensajes MIDI personalizados a dispositivos conectados",
      },
    },
  },
  lng: "en", // if you're using a language detector, do not define the lng option
  fallbackLng: "en",

  interpolation: {
    escapeValue: false, // react already safes from xss => https://www.i18next.com/translation-function/interpolation#unescape
  },
});
