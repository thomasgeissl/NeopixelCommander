import { Box, Button, Typography } from "@mui/material";

const GettingStarted = () => {
  return (
    <Box>
      <Typography variant="h2" gutterBottom>
        Getting Started
      </Typography>

      <Typography variant="body1" gutterBottom>
        Neopixel Commander is an Arduino Library that allows you to control
        Neopixel (WS2812) LED strips via a web interface - namely a rest API and
        a websocket API. It ships with an embedded js engine that allows to run
        stored JS code on the microcontroller itself.
      </Typography>

      <Typography variant="body1" gutterBottom>
        Neopixel Commander is Neopixel-Blocks best friend.
      </Typography>
      <Button variant="contained" color="primary" href="http://neopixel-blocks.cc" target="_blank" rel="noopener" sx={{ mt: 2 }}>
        Visit Neopixel-Blocks Website
      </Button>
    </Box>
  );
};

export default GettingStarted;
