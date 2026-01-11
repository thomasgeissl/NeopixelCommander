import { Box } from "@mui/material";
import Header from "./components/Header";
import Home from "./components/Home";
import { Route, Routes } from "react-router-dom";
import Debugger from "./components/Debugger";
import NotFound from "./components/NotFound";
import ApiDocumentation from "./components/ApiDoc";

function App() {
  return (
    <Box
      width={"100vw"}
      height={"100vh"}
      display={"flex"}
      flexDirection={"column"}
    >
      <Header />
      <Box padding={2} flex={1} display={"flex"}>
        <Routes>
          <Route path="/" element={<Home />} />
          <Route path="/api" element={<ApiDocumentation />} />
          <Route path="/debugger" element={<Debugger />} />
          <Route path="*" element={<NotFound />} />
        </Routes>
      </Box>
    </Box>
  );
}

export default App;
