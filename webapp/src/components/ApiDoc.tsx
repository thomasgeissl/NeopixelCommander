import React, { useState } from 'react';
import {
  Box,
  Typography,
  Accordion,
  AccordionSummary,
  AccordionDetails,
  Chip,
  Paper,
  IconButton,
  Tooltip,
  Card,
  Stack,
  Divider,
  Alert,
  Tabs,
  Tab,
} from '@mui/material';
import {
  ExpandMore as ExpandMoreIcon,
  ContentCopy as ContentCopyIcon,
  Check as CheckIcon,
  Http as HttpIcon,
  Cable as CableIcon,
} from '@mui/icons-material';

interface Parameter {
  name: string;
  type: string;
  required: boolean;
  description: string;
  in?: string;
}

interface Response {
  status?: number;
  description: string;
  example: any;
}

interface RestEndpoint {
  path: string;
  method: string;
  description: string;
  deprecated?: boolean;
  note?: string;
  parameters: Parameter[];
  responses: Response[];
}

interface RestCategory {
  category: string;
  endpoints: RestEndpoint[];
}

interface WebSocketCommand {
  name: string;
  description: string;
  deprecated?: boolean;
  note?: string;
  request: string | any;
  requestType: 'text' | 'json';
  parameters?: Parameter[];
  response?: any;
  responses?: Response[];
}

interface WebSocketCategory {
  category: string;
  commands: WebSocketCommand[];
}

interface ApiSpec {
  apiVersion: string;
  title: string;
  description: string;
  baseUrl: string;
  wsUrl: string;
  restEndpoints: RestCategory[];
  wsCommands: WebSocketCategory[];
}

const apiSpec: ApiSpec = {
  apiVersion: "1.0.0",
  title: "NeopixelCommander API",
  description: "REST and WebSocket API for controlling NeoPixel LED strips via ESP32",
  baseUrl: "http://<device-ip>",
  wsUrl: "ws://<device-ip>/ws",
  restEndpoints: [
    {
      category: "Health & System",
      endpoints: [
        {
          path: "/ping",
          method: "GET",
          description: "Check if the device is responsive",
          parameters: [],
          responses: [
            {
              status: 200,
              description: "Success",
              example: { status: "ok", message: "pong" }
            }
          ]
        },
        {
          path: "/api/pixelCount",
          method: "GET",
          description: "Get the total number of NeoPixels in the strip",
          parameters: [],
          responses: [
            {
              status: 200,
              description: "Success",
              example: { status: "ok", pixelCount: 60 }
            }
          ]
        }
      ]
    },
    {
      category: "LED Control",
      endpoints: [
        {
          path: "/api/setColor",
          method: "POST",
          description: "Set all pixels to the same RGB color",
          parameters: [
            { name: "r", type: "integer", required: true, description: "Red value (0-255)", in: "formData" },
            { name: "g", type: "integer", required: true, description: "Green value (0-255)", in: "formData" },
            { name: "b", type: "integer", required: true, description: "Blue value (0-255)", in: "formData" }
          ],
          responses: [
            { status: 200, description: "Success", example: { status: "ok" } },
            { status: 400, description: "Missing parameters", example: { status: "error", error: "missing_params" } }
          ]
        },
        {
          path: "/api/clear",
          method: "POST",
          description: "Clear all pixels (set to black/off)",
          parameters: [],
          responses: [{ status: 200, description: "Success", example: { status: "ok" } }]
        },
        {
          path: "/api/show",
          method: "POST",
          description: "Update the LED strip to display the current pixel colors",
          parameters: [],
          responses: [{ status: 200, description: "Success", example: { status: "ok" } }]
        },
        {
          path: "/api/setBrightness",
          method: "POST",
          description: "Set the global brightness level",
          parameters: [
            { name: "brightness", type: "integer", required: true, description: "Brightness value (0-255)", in: "formData" }
          ],
          responses: [
            { status: 200, description: "Success", example: { status: "ok" } },
            { status: 400, description: "Missing parameter", example: { status: "error", error: "missing_param" } }
          ]
        }
      ]
    },
    {
      category: "JavaScript Code Management",
      endpoints: [
        {
          path: "/api/savejs",
          method: "POST",
          description: "Save JavaScript code to device preferences for automatic execution on boot",
          parameters: [
            { name: "code", type: "string", required: true, description: "JavaScript code to save", in: "formData or body" }
          ],
          responses: [
            { status: 200, description: "Success", example: { status: "ok" } },
            { status: 400, description: "No code received", example: { status: "error", error: "no_code_received" } },
            { status: 500, description: "Save failed", example: { status: "error", error: "save_failed" } }
          ]
        }
      ]
    }
  ],
  wsCommands: [
    {
      category: "Connection & Health",
      commands: [
        {
          name: "ping (text)",
          description: "Send a simple text message 'ping' (not JSON)",
          request: "ping",
          requestType: "text",
          response: { status: "ok", message: "pong" }
        },
        {
          name: "ping (JSON)",
          description: "Send a ping command as JSON",
          request: { cmd: "ping" },
          requestType: "json",
          response: { status: "ok", message: "pong" }
        },
        {
          name: "getPixelCount",
          description: "Get the number of pixels in the strip",
          request: { cmd: "getPixelCount" },
          requestType: "json",
          response: { status: "ok", pixelCount: 60 }
        }
      ]
    },
    {
      category: "Configuration",
      commands: [
        {
          name: "setCredentials",
          description: "Set WiFi network credentials and save to preferences",
          request: { cmd: "setCredentials", ssid: "YourNetworkName", password: "YourPassword" },
          requestType: "json",
          parameters: [
            { name: "ssid", type: "string", required: true, description: "WiFi network SSID" },
            { name: "password", type: "string", required: true, description: "WiFi network password" }
          ],
          responses: [
            { description: "Success", example: { status: "ok" } },
            { description: "Error - missing credentials", example: { status: "error", error: "missing_credentials" } }
          ]
        }
      ]
    },
    {
      category: "LED Control",
      commands: [
        {
          name: "setColor",
          description: "Set all pixels to the same RGB color",
          note: "Command ID is required and must be unique",
          request: { cmd: "setColor", id: 1, r: 255, g: 0, b: 0 },
          requestType: "json",
          parameters: [
            { name: "id", type: "integer", required: true, description: "Unique command identifier" },
            { name: "r", type: "integer", required: true, description: "Red value (0-255)" },
            { name: "g", type: "integer", required: true, description: "Green value (0-255)" },
            { name: "b", type: "integer", required: true, description: "Blue value (0-255)" }
          ],
          response: { status: "ok", ack: 1 }
        },
        {
          name: "setPixelColor",
          description: "Set a specific pixel to an RGB color",
          request: { cmd: "setPixelColor", id: 2, index: 0, r: 255, g: 0, b: 0 },
          requestType: "json",
          parameters: [
            { name: "id", type: "integer", required: true, description: "Unique command identifier" },
            { name: "index", type: "integer", required: true, description: "Pixel index (0-based)" },
            { name: "r", type: "integer", required: true, description: "Red value (0-255)" },
            { name: "g", type: "integer", required: true, description: "Green value (0-255)" },
            { name: "b", type: "integer", required: true, description: "Blue value (0-255)" }
          ],
          responses: [
            { description: "Success", example: { status: "ok", ack: 2 } },
            { description: "Index out of bounds", example: { status: "error", error: "index_out_of_bounds", id: 2, max: 59 } }
          ]
        },
        {
          name: "clear",
          description: "Clear all pixels (set to black/off)",
          request: { cmd: "clear", id: 3 },
          requestType: "json",
          parameters: [
            { name: "id", type: "integer", required: true, description: "Unique command identifier" }
          ],
          response: { status: "ok", ack: 3 }
        },
        {
          name: "show",
          description: "Update the LED strip to display current pixel colors",
          request: { cmd: "show", id: 4 },
          requestType: "json",
          parameters: [
            { name: "id", type: "integer", required: true, description: "Unique command identifier" }
          ],
          response: { status: "ok", ack: 4 }
        },
        {
          name: "setBrightness",
          description: "Set the global brightness level",
          request: { cmd: "setBrightness", id: 5, brightness: 128 },
          requestType: "json",
          parameters: [
            { name: "id", type: "integer", required: true, description: "Unique command identifier" },
            { name: "brightness", type: "integer", required: true, description: "Brightness value (0-255)" }
          ],
          response: { status: "ok", ack: 5 }
        }
      ]
    },
    {
      category: "JavaScript Execution",
      commands: [
        {
          name: "saveJS",
          description: "Save JavaScript code to device preferences",
          request: { cmd: "saveJS", id: 6, code: "setAllPixelColor(0, 255, 0); show();" },
          requestType: "json",
          parameters: [
            { name: "id", type: "integer", required: true, description: "Unique command identifier" },
            { name: "code", type: "string", required: true, description: "JavaScript code to save" }
          ],
          responses: [
            { description: "Success", example: { status: "ok", ack: 6 } },
            { description: "Missing code", example: { status: "error", error: "missing_code" } },
            { description: "Save failed", example: { status: "error", error: "save_failed" } }
          ]
        }
      ]
    }
  ]
};

const CodeBlock: React.FC<{ code: string | any }> = ({ code }) => {
  const [copied, setCopied] = useState(false);
  
  const handleCopy = () => {
    const codeString = typeof code === 'string' ? code : JSON.stringify(code, null, 2);
    navigator.clipboard.writeText(codeString);
    setCopied(true);
    setTimeout(() => setCopied(false), 2000);
  };

  const codeString = typeof code === 'string' ? code : JSON.stringify(code, null, 2);

  return (
    <Box sx={{ position: 'relative' }}>
      <Tooltip title={copied ? "Copied!" : "Copy to clipboard"}>
        <IconButton onClick={handleCopy} sx={{ position: 'absolute', top: 8, right: 8, zIndex: 1 }} size="small">
          {copied ? <CheckIcon fontSize="small" /> : <ContentCopyIcon fontSize="small" />}
        </IconButton>
      </Tooltip>
      <Paper sx={{ bgcolor: '#1e1e1e', p: 2, overflow: 'auto', fontFamily: 'monospace', fontSize: '0.875rem', color: '#d4d4d4' }}>
        <pre style={{ margin: 0 }}><code>{codeString}</code></pre>
      </Paper>
    </Box>
  );
};

const MethodChip: React.FC<{ method: string }> = ({ method }) => {
  const colors: Record<string, any> = {
    GET: { bgcolor: '#2196f3', color: 'white' },
    POST: { bgcolor: '#4caf50', color: 'white' },
    PUT: { bgcolor: '#ff9800', color: 'white' },
    DELETE: { bgcolor: '#f44336', color: 'white' },
  };
  const style = colors[method] || { bgcolor: '#9e9e9e', color: 'white' };
  return <Chip label={method} size="small" sx={{ ...style, fontWeight: 600, fontSize: '0.75rem' }} />;
};

const EndpointCard: React.FC<{ endpoint: RestEndpoint }> = ({ endpoint }) => {
  return (
    <Accordion sx={{ mb: 1.5 }}>
      <AccordionSummary expandIcon={<ExpandMoreIcon />}>
        <Stack direction="row" spacing={2} alignItems="center">
          <MethodChip method={endpoint.method} />
          <Typography variant="body2" component="code" sx={{ fontFamily: 'monospace', color: 'primary.main' }}>
            {endpoint.path}
          </Typography>
          {endpoint.deprecated && <Chip label="DEPRECATED" color="warning" size="small" />}
        </Stack>
      </AccordionSummary>
      <AccordionDetails>
        <Stack spacing={3}>
          <Typography variant="body2" color="text.secondary">{endpoint.description}</Typography>
          {endpoint.note && <Alert severity="warning" sx={{ fontSize: '0.875rem' }}>{endpoint.note}</Alert>}
          {endpoint.parameters.length > 0 && (
            <Box>
              <Typography variant="subtitle2" gutterBottom fontWeight={600}>Parameters</Typography>
              <Stack spacing={1.5}>
                {endpoint.parameters.map((param, idx) => (
                  <Paper key={idx} variant="outlined" sx={{ p: 2 }}>
                    <Stack direction="row" spacing={1} alignItems="center" mb={0.5}>
                      <Typography component="code" variant="body2" sx={{ fontFamily: 'monospace', color: 'secondary.main', fontWeight: 600 }}>
                        {param.name}
                      </Typography>
                      <Chip label={param.type} size="small" variant="outlined" />
                      {param.required && <Chip label="required" color="error" size="small" />}
                      <Typography variant="caption" color="text.secondary">in: {param.in}</Typography>
                    </Stack>
                    <Typography variant="body2" color="text.secondary">{param.description}</Typography>
                  </Paper>
                ))}
              </Stack>
            </Box>
          )}
          <Box>
            <Typography variant="subtitle2" gutterBottom fontWeight={600}>Responses</Typography>
            <Stack spacing={2}>
              {endpoint.responses.map((response, idx) => (
                <Box key={idx}>
                  <Stack direction="row" spacing={1} alignItems="center" mb={1}>
                    <Chip label={response.status} size="small" color={response.status && response.status >= 200 && response.status < 300 ? 'success' : 'error'} sx={{ fontWeight: 600 }} />
                    <Typography variant="body2" color="text.secondary">{response.description}</Typography>
                  </Stack>
                  {response.example && <CodeBlock code={response.example} />}
                </Box>
              ))}
            </Stack>
          </Box>
        </Stack>
      </AccordionDetails>
    </Accordion>
  );
};

const WebSocketCommandCard: React.FC<{ command: WebSocketCommand }> = ({ command }) => {
  return (
    <Accordion sx={{ mb: 1.5 }}>
      <AccordionSummary expandIcon={<ExpandMoreIcon />}>
        <Stack direction="row" spacing={2} alignItems="center">
          <Chip label="WS" size="small" sx={{ bgcolor: '#9c27b0', color: 'white', fontWeight: 600, fontSize: '0.75rem' }} />
          <Typography variant="body2" component="code" sx={{ fontFamily: 'monospace', color: 'primary.main', fontWeight: 600 }}>
            {command.name}
          </Typography>
          {command.deprecated && <Chip label="DEPRECATED" color="warning" size="small" />}
        </Stack>
      </AccordionSummary>
      <AccordionDetails>
        <Stack spacing={3}>
          <Typography variant="body2" color="text.secondary">{command.description}</Typography>
          {command.note && <Alert severity="info" sx={{ fontSize: '0.875rem' }}>{command.note}</Alert>}
          {command.parameters && command.parameters.length > 0 && (
            <Box>
              <Typography variant="subtitle2" gutterBottom fontWeight={600}>Parameters</Typography>
              <Stack spacing={1.5}>
                {command.parameters.map((param, idx) => (
                  <Paper key={idx} variant="outlined" sx={{ p: 2 }}>
                    <Stack direction="row" spacing={1} alignItems="center" mb={0.5}>
                      <Typography component="code" variant="body2" sx={{ fontFamily: 'monospace', color: 'secondary.main', fontWeight: 600 }}>
                        {param.name}
                      </Typography>
                      <Chip label={param.type} size="small" variant="outlined" />
                      {param.required && <Chip label="required" color="error" size="small" />}
                    </Stack>
                    <Typography variant="body2" color="text.secondary">{param.description}</Typography>
                  </Paper>
                ))}
              </Stack>
            </Box>
          )}
          <Box>
            <Typography variant="subtitle2" gutterBottom fontWeight={600}>
              Request {command.requestType === 'text' && '(Plain Text)'}
            </Typography>
            <CodeBlock code={command.request} />
          </Box>
          {command.response && (
            <Box>
              <Typography variant="subtitle2" gutterBottom fontWeight={600}>Response</Typography>
              <CodeBlock code={command.response} />
            </Box>
          )}
          {command.responses && (
            <Box>
              <Typography variant="subtitle2" gutterBottom fontWeight={600}>Responses</Typography>
              <Stack spacing={2}>
                {command.responses.map((response, idx) => (
                  <Box key={idx}>
                    <Typography variant="body2" color="text.secondary" mb={1}>{response.description}</Typography>
                    {response.example && <CodeBlock code={response.example} />}
                  </Box>
                ))}
              </Stack>
            </Box>
          )}
        </Stack>
      </AccordionDetails>
    </Accordion>
  );
};

const CategorySection: React.FC<{ category: string; endpoints: RestEndpoint[] }> = ({ category, endpoints }) => {
  return (
    <Box sx={{ mb: 4 }}>
      <Typography variant="h6" gutterBottom fontWeight={600} sx={{ mb: 2 }}>{category}</Typography>
      <Box>{endpoints.map((endpoint, idx) => <EndpointCard key={idx} endpoint={endpoint} />)}</Box>
    </Box>
  );
};

const WebSocketCategorySection: React.FC<{ category: string; commands: WebSocketCommand[] }> = ({ category, commands }) => {
  return (
    <Box sx={{ mb: 4 }}>
      <Typography variant="h6" gutterBottom fontWeight={600} sx={{ mb: 2 }}>{category}</Typography>
      <Box>{commands.map((command, idx) => <WebSocketCommandCard key={idx} command={command} />)}</Box>
    </Box>
  );
};

function TabPanel({ children, value, index }: { children?: React.ReactNode; value: number; index: number }) {
  return (
    <Box role="tabpanel" hidden={value !== index}>
      {value === index && <Box sx={{ py: 3 }}>{children}</Box>}
    </Box>
  );
}

export default function ApiDocumentation() {
  const [tabValue, setTabValue] = useState(0);

  return (
    <Box sx={{ minHeight: '100vh', bgcolor: 'background.default', p: 3 }}>
      <Box maxWidth={"lg"} sx={{ mx: 'auto' }}>
        <Card sx={{ mb: 4, p: 3 }}>
          <Typography variant="h4" gutterBottom fontWeight={700}>{apiSpec.title}</Typography>
          <Typography variant="body1" color="text.secondary" paragraph>{apiSpec.description}</Typography>
          <Divider sx={{ my: 2 }} />
          <Stack spacing={1}>
            <Stack direction="row" spacing={2} alignItems="center">
              <Typography variant="body2" color="text.secondary">Version:</Typography>
              <Chip label={apiSpec.apiVersion} size="small" variant="outlined" />
            </Stack>
            <Stack direction="row" spacing={2} alignItems="center">
              <Typography variant="body2" color="text.secondary">REST Base URL:</Typography>
              <Typography component="code" variant="body2" sx={{ fontFamily: 'monospace', bgcolor: 'action.hover', px: 1, py: 0.5, borderRadius: 1, color: 'primary.main' }}>
                {apiSpec.baseUrl}
              </Typography>
            </Stack>
            <Stack direction="row" spacing={2} alignItems="center">
              <Typography variant="body2" color="text.secondary">WebSocket URL:</Typography>
              <Typography component="code" variant="body2" sx={{ fontFamily: 'monospace', bgcolor: 'action.hover', px: 1, py: 0.5, borderRadius: 1, color: 'secondary.main' }}>
                {apiSpec.wsUrl}
              </Typography>
            </Stack>
          </Stack>
        </Card>

        <Paper sx={{ mb: 3 }}>
          <Tabs value={tabValue} onChange={(_, v) => setTabValue(v)} variant="fullWidth" sx={{ borderBottom: 1, borderColor: 'divider' }}>
            <Tab icon={<HttpIcon />} iconPosition="start" label="REST API" sx={{ fontWeight: 600 }} />
            <Tab icon={<CableIcon />} iconPosition="start" label="WebSocket API" sx={{ fontWeight: 600 }} />
          </Tabs>
        </Paper>

        <TabPanel value={tabValue} index={0}>
          {apiSpec.restEndpoints.map((section, idx) => (
            <CategorySection key={idx} category={section.category} endpoints={section.endpoints} />
          ))}
        </TabPanel>

        <TabPanel value={tabValue} index={1}>
          {apiSpec.wsCommands.map((section, idx) => (
            <WebSocketCategorySection key={idx} category={section.category} commands={section.commands} />
          ))}
        </TabPanel>
      </Box>
    </Box>
  );
}