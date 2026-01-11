import {
  Box,
  Button,
  Stack,
} from "@mui/material";
import {
  UsbOff as UsbOffIcon,
  Usb as UsbIcon,
} from "@mui/icons-material";
import { useTranslation } from "react-i18next";
import { useSerialStore } from "../stores/serial";
import Console from "./SerialConsole";


export default function SerialMonitor() {
  const { isConnected, connect, disconnect } =
    useSerialStore();

  const { t } = useTranslation();


  return (
    <Box sx={{ p: 1 }}>
      <Stack spacing={2}>
        <Box sx={{ display: "flex", gap: 1 }}>
          {!isConnected ? (
            <Button
              variant="contained"
              onClick={connect}
              startIcon={<UsbIcon />}
            >
              {t("connect")}
            </Button>
          ) : (
            <Button
              variant="contained"
              color="error"
              onClick={disconnect}
              startIcon={<UsbOffIcon />}
            >
              {t("disconnect")}
            </Button>
          )}

          <Box flex={1} />
        </Box>

        <Console canSend={isConnected} height={400} />
      </Stack>
    </Box>
  );
}
