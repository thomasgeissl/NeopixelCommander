import { useEffect, useRef, useState } from "react";
import {
  Box,
  TextField,
  Paper,
  Typography,
  IconButton,
  Button,
} from "@mui/material";
import { Send } from "@mui/icons-material";
import { useSerialStore } from "../stores/serial";
import { Close as CloseIcon } from "@mui/icons-material";
import { useTranslation } from "react-i18next";

type LogType = "send" | "receive" | "error" | "system";

interface ConsoleProps {
  canSend?: boolean;
  placeholder?: string;
  height?: number | string;
}

export default function Console({
  canSend = false,
  placeholder = "Type message...",
  height = 400,
}: ConsoleProps) {
  const { send, log, clearLog } = useSerialStore();
  const [input, setInput] = useState("");
  const logEndRef = useRef<HTMLDivElement>(null);
  const { t } = useTranslation();

  useEffect(() => {
    logEndRef.current?.scrollIntoView({ behavior: "smooth" });
  }, [log]);

  const getLogColor = (type: LogType) => {
    switch (type) {
      case "send":
        return "primary.main";
      case "receive":
        return "text.primary";
      case "error":
        return "error.main";
      case "system":
        return "text.secondary";
      default:
        return "text.primary";
    }
  };

  const handleSend = async () => {
    if (!input.trim()) return;
    await send(input);
    setInput("");
  };

  return (
    <Box mt={2}>
      <Box my={1} display="flex" gap={1}>
        <Typography variant="h6" flex={1}>
          {t("console")}
        </Typography>
        <Box flex={1} />
        <Button
          variant="contained"
          color="error"
          onClick={clearLog}
          startIcon={<CloseIcon />}
        >
          {t("clear")}
        </Button>
      </Box>
      <Paper sx={{ p: 2, height, overflow: "auto" }}>
        {log.length === 0 ? (
          <Typography color="text.secondary" sx={{ fontFamily: "monospace" }}>
            No data yet...
          </Typography>
        ) : (
          log.map((entry, idx) => (
            <Typography
              key={idx}
              sx={{
                fontFamily: "monospace",
                fontSize: "0.875rem",
                color: getLogColor(entry.type),
                whiteSpace: "pre-wrap",
                wordBreak: "break-word",
              }}
            >
              {entry.type === "send" && "> "}
              {entry.message}
            </Typography>
          ))
        )}
        <div ref={logEndRef} />
      </Paper>

      {canSend && (
        <Box sx={{ display: "flex", gap: 1, mt: 2 }}>
          <TextField
            fullWidth
            value={input}
            onChange={(e) => setInput(e.target.value)}
            onKeyDown={(e) => {
              if (e.key === "Enter") handleSend();
            }}
            placeholder={placeholder}
          />
          <IconButton onClick={handleSend} disabled={!input.trim()}>
            <Send />
          </IconButton>
        </Box>
      )}
    </Box>
  );
}
