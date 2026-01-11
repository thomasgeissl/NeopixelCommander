import React from "react";
import AppBar from "@mui/material/AppBar";
import Toolbar from "@mui/material/Toolbar";
import Typography from "@mui/material/Typography";
import Box from "@mui/material/Box";
import { Settings as SettingsIcon } from "@mui/icons-material";

import { Button, IconButton } from "@mui/material";
import { NavLink, useNavigate } from "react-router-dom";

const Header: React.FC = () => {
  const navigate = useNavigate();
  const menu = [
    { label: "Configurator", path: "/configurator" },
    { label: "Debugger", path: "/debugger" },
    // { label: "Inspector", path: "/inspector" },
    // { label: "Firmware Uploader", path: "/uploader" },
  ];

  return (
    <AppBar position="static" color="transparent" elevation={2}>
      <Toolbar sx={{ display: "flex", gap: 2 }}>
        <Box display={"flex"} alignItems="center" gap={1}>
          <Typography
            variant="h6"
            component="div"
            sx={{ flexGrow: 1, fontWeight: 600, cursor: "pointer" }}
            textTransform="uppercase"
            onClick={() => navigate("/")}
            color="primary"
          >
            NeopixelCommander
          </Typography>
        </Box>
        <Box flex={1} />

        <Box sx={{ display: "flex", alignItems: "center", gap: 1 }}>
          {menu.map((item) => (
            <Button
              key={item.path}
              component={NavLink}
              to={item.path}
              color="inherit"
              sx={{
                "&.active": {
                  backgroundColor: "primary.main",
                  color: "primary.contrastText",
                  fontWeight: 600,
                },
              }}
            >
              {item.label}
            </Button>
          ))}
          <IconButton
            onClick={() => {}}
            color="inherit"
          >
            <SettingsIcon />
          </IconButton>
        </Box>
      </Toolbar>
    </AppBar>
  );
};

export default Header;
