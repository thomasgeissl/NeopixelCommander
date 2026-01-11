import { Box } from "@mui/material"
import Header from "./components/Header"
import Home from "./components/Home"

function App() {

  return (
    <Box width={"100vw"} height={"100vh"} display={"flex"} flexDirection={"column"}>
      <Header />
      <Box padding={2} flex={1} display={"flex"}>
        <Home></Home>
      </Box>
    </Box>
  )
}

export default App
