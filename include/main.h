#define EEPROM_SIZE 1024

String ProjectName = "";
String MainVersion = "2024-06-04.01";
String SerialNumber = "";

uint64_t ServerHandleClientTimer = 0;
uint32_t ServerConfigurationPauseTime = 1000 * 60 * 5; // 5 минут
String WebContent;                                     // формируемая страница сервера

char SSID[33] = {0};
char PASS[33] = {0};
uint8_t IP[4] = {0};
uint8_t GW[4] = {0};
uint8_t MASK[4] = {0};
uint8_t DNS[4] = {0};
uint8_t UseDHCP = 0;

// ************************* EEPROM MAP *************************
uint8_t adrSSID = 0;  // 0 - 31
uint8_t adrPASS = 32; // 32 - 63
uint8_t adrIP = 64;   // 64  - 67  ip0 - ip3
uint8_t adrGW = 68;   // 68  - 71  GW0 - GW3
uint8_t adrDNS = 72;  // 72  - 75  DNS0 - DNS3
uint8_t adrMASK = 76; // 76  - 79  Subnet MASK
// uint8_t adrServerAddress = 81; // 81  - 198 Server address
// uint8_t adrServerPort = 190;   // 199       Server port
uint8_t adrUseDHCP = 190; // Use DHCP mode
uint8_t adrLIC = 191;     // Licensed
uint8_t adrScreenUpdatePeriod = 192; // Период обновления экрана
uint8_t adrAlarmBlokingTime = 196;  // Время отключения сигнала мс


uint8_t adrOutputsConf = 200; // 200 - ??? Outputs conf

char DebugMessage[100] = {0};

/* Put IP Address details */
IPAddress local_ip(192, 168, 0, 100);
IPAddress gateway(192, 168, 0, 100);
IPAddress subnet(255, 255, 255, 0);

void DebugMSG(char *Msg);
void ResetMCU(void);
void Device_Info(void);
void send_MainPage(void);
void GetSerialNumber(void);
void SetProjectName(void);
void PreferencesRead(void);
void PreferencesWrite(void);
void SetStartUpPreferences(void);
void SendSetupPage(void);
void SendStartSetupPage(void);
void StartWorkServer();
void StartSetupServer();
void SetupWebServerWork();

const char *loginIndex =
    "<form name='loginForm'>"
    "<table width='20%' bgcolor='A09F9F' align='center'>"
    "<tr>"
    "<td colspan=2>"
    "<center><font size=4><b>ESP32 Login Page</b></font></center>"
    "<br>"
    "</td>"
    "<br>"
    "<br>"
    "</tr>"
    "<tr>"
    "<td>Username:</td>"
    "<td><input type='text' size=25 name='userid'><br></td>"
    "</tr>"
    "<br>"
    "<br>"
    "<tr>"
    "<td>Password:</td>"
    "<td><input type='Password' size=25 name='pwd'><br></td>"
    "<br>"
    "<br>"
    "</tr>"
    "<tr>"
    "<td><input type='submit' onclick='check(this.form)' value='Login'></td>"
    "</tr>"
    "</table>"
    "</form>"
    "<script>"
    "function check(form)"
    "{"
    "if(form.userid.value=='admin' && form.pwd.value=='admin')"
    "{"
    "window.open('/firmwareUpdate')"
    "}"
    "else"
    "{"
    " alert('Error Password or Username')/*displays error message*/"
    "}"
    "}"
    "</script>";

const char *firmwareUpdate =
    "<script src='https://ajax.googleapis.com/ajax/libs/jquery/3.2.1/jquery.min.js'></script>"
    "<form method='POST' action='#' enctype='multipart/form-data' id='upload_form'>"
    "<input type='file' name='update'>"
    "<input type='submit' value='Update'>"
    "</form>"
    "<div id='prg'>progress: 0%</div>"
    "<script>"
    "$('form').submit(function(e){"
    "e.preventDefault();"
    "var form = $('#upload_form')[0];"
    "var data = new FormData(form);"
    " $.ajax({"
    "url: '/update',"
    "type: 'POST',"
    "data: data,"
    "contentType: false,"
    "processData:false,"
    "xhr: function() {"
    "var xhr = new window.XMLHttpRequest();"
    "xhr.upload.addEventListener('progress', function(evt) {"
    "if (evt.lengthComputable) {"
    "var per = evt.loaded / evt.total;"
    "$('#prg').html('progress: ' + Math.round(per*100) + '%');"
    "}"
    "}, false);"
    "return xhr;"
    "},"
    "success:function(d, s) {"
    "console.log('success!')"
    "},"
    "error: function (a, b, c) {"
    "}"
    "});"
    "});"
    "</script>";

