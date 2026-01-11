// theme.ts
import { createTheme } from "@mui/material/styles";

const theme = createTheme({
  palette: {
    mode: "dark",

    // muted rose-gold
    primary: {
      main: "#d89cae",
    },

    // dusty mauve accent
    secondary: {
      main: "#b7a4c6",
    },

    background: {
      // warm charcoal rather than blue charcoal
      default: "#1a1718",
      paper: "#231f21",
    },

    text: {
      primary: "#f0eceb",
      secondary: "#c7bfc3",
    },

    divider: "rgba(255,255,255,0.08)",
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

  // shadows: Array(25).fill("0 4px 14px rgba(0, 0, 0, 0.32)"),

  components: {
    MuiCssBaseline: {
      styleOverrides: `
        body {
          background-color: #1a1718;
          background-image:
            radial-gradient(circle at 45% 18%, rgba(255,245,240,0.05) 0%, transparent 60%),
            radial-gradient(circle at 15% 75%, rgba(255,235,230,0.04) 0%, transparent 70%);
        }
      `,
    },

    MuiPaper: {
      styleOverrides: {
        root: {
          backgroundColor: "#231f21",
          border: "1px solid rgba(255,255,255,0.06)",
          boxShadow: "0 4px 14px rgba(0,0,0,0.32)",
        },
      },
    },

    MuiCard: {
      styleOverrides: {
        root: {
          backgroundColor: "#231f21",
          border: "1px solid rgba(255,255,255,0.06)",
          boxShadow: "0 4px 14px rgba(0,0,0,0.32)",
        },
      },
    },

    MuiButton: {
      defaultProps: {
        disableElevation: true,
      },
      styleOverrides: {
        root: {
          borderRadius: 12,
          padding: "0.55rem 1.2rem",
          backgroundColor: "#2f272a",
          color: "#f7f3f2",
          border: "1px solid rgba(255,255,255,0.08)",
          fontWeight: 500,
          "&:hover": {
            backgroundColor: "#3a3034",
            border: "1px solid rgba(255,255,255,0.12)",
          },
        },
      },
    },

    MuiAppBar: {
      styleOverrides: {
        root: {
          backgroundColor: "#231f21",
          borderBottom: "1px solid rgba(255,255,255,0.07)",
        },
      },
    },
  },
});

export default theme;
