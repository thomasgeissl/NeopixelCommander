// theme.ts
import { createTheme } from "@mui/material/styles";

const theme = createTheme({
  palette: {
    mode: "light",

    primary: {
      main: "#c98a9f",
    },

    secondary: {
      main: "#a89bbd",
    },

    background: {
      default: "#f6f3f4",
      paper: "#ffffff",
    },

    text: {
      primary: "#2f2a2c",
      secondary: "#6f666a",
    },

    divider: "rgba(0,0,0,0.08)",
  },

  typography: {
    fontFamily: `"Inter", "Roboto", sans-serif`,
    h1: {
      fontSize: "2.3rem",
      fontWeight: 600,
      letterSpacing: "0.4px",
      marginBottom: "1.2rem",
    },
    h2: {
      fontSize: "1.85rem",
      fontWeight: 600,
      marginBottom: "1rem",
    },
    h3: {
      fontSize: "1.35rem",
      fontWeight: 500,
      marginBottom: "0.75rem",
    },
    body1: {
      lineHeight: 1.55,
    },
  },

  shape: {
    borderRadius: 14,
  },
});

export default theme;
