import {
  Box,
  Button,
  IconButton,
  Stack,
  TextField,
  Typography,
} from "@mui/material";
import {
  UsbOff as UsbOffIcon,
  Usb as UsbIcon,
  Send,
} from "@mui/icons-material";
import { useTranslation } from "react-i18next";
import { useSerialStore } from "../stores/serial";
import Console from "./SerialConsole";
import { useState } from "react";

export default function SerialMonitor() {
  const { isConnected, connect, disconnect, send } = useSerialStore();

  const { t } = useTranslation();
  const [ssid, setSsid] = useState("");
  const [password, setPassword] = useState("");

  const handleSendCredentials = async () => {
    if (!ssid.trim() && !password.trim()) return;
    const payload = {
      cmd: "setCredentials",
      id: 1,
      ssid: ssid,
      password: password,
    };

    await send(JSON.stringify(payload));
    setSsid("");
    setPassword("");
  };
  const handleGetWifiStatus = async () => {
    const payload = {
      cmd: "getCredentials",
      id: 1,
    };
    await send(JSON.stringify(payload));
  }
  const handlePing = async () => {
    const payload = {
      cmd: "ping",
      id: 1,
    };
    await send(JSON.stringify(payload));
  }

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
        {isConnected && (
          <Box display={"flex"} flexDirection="column" gap={1}>
            <Box display={"flex"} gap={1}>
              <Button
                onClick={handleGetWifiStatus}
                variant="outlined"
                color="primary"
              >
                {t("get_wifi_status")}
              </Button>
              <Button
                onClick={handlePing}
                variant="outlined"
                color="primary"
              >
                {t("ping")}
              </Button>
            </Box>
            <Box display={"flex"} alignItems="center" gap={1}>
              <Typography variant="subtitle1" sx={{}}>
                set wifi credentials
              </Typography>
              <TextField
                label="WiFi SSID"
                variant="outlined"
                size="small"
                value={ssid}
                onChange={(e) => setSsid(e.target.value)}
                fullWidth
              />
              <TextField
                label="WiFi Password"
                variant="outlined"
                size="small"
                value={password}
                onChange={(e) => setPassword(e.target.value)}
                fullWidth
              />
              <IconButton
                onClick={handleSendCredentials}
                disabled={!ssid.trim() && !password.trim()}
                color="primary"
              >
                <Send />
              </IconButton>
            </Box>
          </Box>
        )}
      </Stack>
    </Box>
  );
}
