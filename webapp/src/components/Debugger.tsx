import { Box } from "@mui/material";
import SerialMonitor from "./SerialMonitor";

const Debugger = () => {
  return (
    <Box flex={1}>
      <SerialMonitor></SerialMonitor>
    </Box>
  );
};

export default Debugger;
