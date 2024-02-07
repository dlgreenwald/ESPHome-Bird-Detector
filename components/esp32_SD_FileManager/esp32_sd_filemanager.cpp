#include "esp32_sd_filemanager.h"

#include <Arduino.h>

#include "esphome/core/application.h"
#include "esphome/core/hal.h"
#include "esphome/core/helpers.h"
#include "esphome/core/log.h"
#include "esphome/core/util.h"

#include <cstdlib>
#include <esp_http_server.h>
#include <utility>

#include <FS.h>
#include <SD.h>
#include <LittleFS.h>
#include <SD_MMC.h>
#include <FFat.h>
#include "esphome/components/esp32_sdmmc/esp32_sdmmc.h"
#include "ESPFMfGKWp.h"
#include "ESPFMfGKWpF2.h"
#include "protocol_examples_utils.h"

namespace esphome {
namespace esp32_sd_filemanager {

static const char *TAG = "esp32_sd_filemanager";

ESP32SDFM::ESP32SDFM() {}

ESP32SDFM::~ESP32SDFM() {}

void ESP32SDFM::setup() {
  if (!esphome::esp32_sdmmc::global_ESP32SDMMC || esphome::esp32_sdmmc::global_ESP32SDMMC->is_failed()) {
    this->mark_failed();
    return;
  }
  if (!this->AddFS(SD_MMC, "SD-MMC-Card", false)) {
      mark_failed();
      ESP_LOGE(TAG, "Adding SD_MMC failed.");
      return;
  }

  this->WebPageTitle = "FileManager";
  this->BackgroundColor = "white";
  this->textareaCharset = "accept-charset=\"utf-8\"";
  
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  config.server_port = this->port_;
  config.ctrl_port = this->port_;
  config.max_open_sockets = 1;
  config.backlog_conn = 2;
  config.lru_purge_enable = true;

  if (httpd_start(&this->httpd_, &config) != ESP_OK) {
    mark_failed();
    return;
  }
  httpd_uri_t index = {
      .uri = "/",
      .method = HTTP_GET,
      .handler = [](struct httpd_req *req) { return ((ESP32SDFM *) req->user_ctx)->index_handler_(req); },
      .user_ctx = this};
  httpd_register_uri_handler(this->httpd_, &index);

  httpd_uri_t css = {
      .uri = "/fm.css",
      .method = HTTP_GET,
      .handler = [](struct httpd_req *req) { return ((ESP32SDFM *) req->user_ctx)->CSS_handler_(req); },
      .user_ctx = this};
  httpd_register_uri_handler(this->httpd_, &css);

  httpd_uri_t js = {
      .uri = "/fm.js",
      .method = HTTP_GET,
      .handler = [](struct httpd_req *req) { return ((ESP32SDFM *) req->user_ctx)->JS_handler_(req); },
      .user_ctx = this};
  httpd_register_uri_handler(this->httpd_, &js);

  httpd_uri_t fileListInsert = {
      .uri = "/i",
      .method = HTTP_GET,
      .handler = [](struct httpd_req *req) { return ((ESP32SDFM *) req->user_ctx)->FileListInsert_handler_(req); },
      .user_ctx = this};
  httpd_register_uri_handler(this->httpd_, &fileListInsert);

  httpd_uri_t bootInfo = {
      .uri = "/b",
      .method = HTTP_GET,
      .handler = [](struct httpd_req *req) { return ((ESP32SDFM *) req->user_ctx)->BootInfo_handler_(req); },
      .user_ctx = this};
  httpd_register_uri_handler(this->httpd_, &bootInfo);

  httpd_uri_t job = {
      .uri = "/job",
      .method = HTTP_GET,
      .handler = [](struct httpd_req *req) { return ((ESP32SDFM *) req->user_ctx)->job_handler_(req); },
      .user_ctx = this};
  httpd_register_uri_handler(this->httpd_, &job);

  httpd_uri_t interface = {
      .uri = "/if",
      .method = HTTP_GET,
      .handler = [](struct httpd_req *req) { return ((ESP32SDFM *) req->user_ctx)->HtmlIncludesInterface_handler_(req); },
      .user_ctx = this};
  httpd_register_uri_handler(this->httpd_, &interface);

  httpd_uri_t ok = {
      .uri = "/r",
      .method = HTTP_GET,
      .handler = [](struct httpd_req *req) { return ((ESP32SDFM *) req->user_ctx)->ReceiverOK_handler_(req); },
      .user_ctx = this};
  httpd_register_uri_handler(this->httpd_, &ok);

}

float ESP32SDFM::get_setup_priority() const { return setup_priority::LATE; }

void ESP32SDFM::loop() {

}

void ESP32SDFM::dump_config() {
    ESP_LOGCONFIG(TAG, "SD File Manager");
    ESP_LOGCONFIG(TAG, "  Port: %d", this->port_);
    if (this->is_failed()) {
        ESP_LOGE(TAG, "  Setup Failed");
    }
}

bool ESP32SDFM::AddFS(fs::FS &fs, String FSname, bool AutoTreemode)
{
  // Add into first slot, if empty. Else always add into second.
  if (maxfilesystem < maxfilesystems)
  {
    fsinfo[maxfilesystem].filesystem = &fs;
    fsinfo[maxfilesystem].fsname = FSname;
    fsinfo[maxfilesystem].AutoTreemode = AutoTreemode;
    maxfilesystem++;
    return true;
  }
  return false;
}

esp_err_t ESP32SDFM::index_handler_(struct httpd_req *req) {
  esp_err_t res = ESP_FAIL;
  
  res = httpd_resp_set_type(req,  "text/html");
  if (res != ESP_OK) {
    ESP_LOGW(TAG, "SNAPSHOT: failed to set HTTP response type");
    return res;
  }
    if (res == ESP_OK) {
    res = httpd_resp_send(req, PSTR(ESPFMfGKWpindexpage), strlen(PSTR(ESPFMfGKWpindexpage)));
  }
  //no need to send empy chunk to signal end as this codepath only has one send.
  return res;
}

esp_err_t ESP32SDFM::HtmlIncludesInterface_handler_(struct httpd_req *req) {
  esp_err_t res = ESP_FAIL;
  
  httpd_resp_set_status(req, HTTPD_404);
  httpd_resp_send(req, NULL, 0);  // Response body can be empty

  //no need to send empy chunk to signal end as this codepath only has one send.
  return res;
}

esp_err_t ESP32SDFM::ReceiverOK_handler_(struct httpd_req *req) {
  esp_err_t res = ESP_FAIL;
  
  httpd_resp_set_status(req, HTTPD_200);
  httpd_resp_send(req, NULL, 0);  // Response body can be empty

  //no need to send empy chunk to signal end as this codepath only has one send.
  return res;
}

esp_err_t ESP32SDFM::CSS_handler_(struct httpd_req *req) {
  esp_err_t res = ESP_FAIL;
  
  res = httpd_resp_set_type(req,  "text/css");
  if (res != ESP_OK) {
    ESP_LOGW(TAG, "SNAPSHOT: failed to set HTTP response type");
    return res;
  }
    if (res == ESP_OK) {
    res = httpd_resp_send(req, PSTR(ESPFMfGKWpcss), strlen(PSTR(ESPFMfGKWpcss)));
  }

  //no need to send empy chunk to signal end as this codepath only has one send.
  return res;
}

esp_err_t ESP32SDFM::JS_handler_(struct httpd_req *req) {
  esp_err_t res = ESP_FAIL;
  
  res = httpd_resp_set_type(req,  "text/javascript");
  if (res != ESP_OK) {
    ESP_LOGW(TAG, "SNAPSHOT: failed to set HTTP response type");
    return res;
  }
    if (res == ESP_OK) {
    res = httpd_resp_send(req, PSTR(ESPFMfGKWpjavascript), strlen(PSTR(ESPFMfGKWpjavascript)));
  }
  //no need to send empy chunk to signal end as this codepath only has one send.
  return res;
}

esp_err_t ESP32SDFM::FileListInsert_handler_(struct httpd_req *req) {
  esp_err_t res = ESP_FAIL;
  queryParams params;

  esphome::esp32_sdmmc::global_ESP32SDMMC->get_sd_lock();

  res = extractQueryParams(req, params);
  if (res != ESP_OK){
    ESP_LOGE(TAG, "Failure to parse query params");
    return res;
  }

  // get the file system. all safe.
  int fsi = getFileSystemIndex(params);
  bool sit = ShowInTreeView(params);
  String path = CurrentPath(params);
  int maxtiefe = 0;
  for (int i = 0; i < path.length(); i++)
  {
    if (path.charAt(i) == '/')
    {
      maxtiefe++;
    }
  }

  // Flat view: immer im Root beginnen
  if ((!sit) || (path == ""))
  {
    path = "/";
  }

  /** /
  Serial.print("fsi: ");
  Serial.print(fsi);
  Serial.print(" sit: ");
  Serial.print(sit);
  Serial.print(" maxtiefe: ");
  Serial.print(maxtiefe);
  Serial.print(" path: ");
  Serial.print(path);
  Serial.println();
  /**/

  res = httpd_resp_set_type(req,  "text/html");
  if (res != ESP_OK) {
    ESP_LOGW(TAG, "SNAPSHOT: failed to set HTTP response type");
    return res;
  }

  gzipperexists = ((fsinfo[fsi].filesystem->exists("/gzipper.js.gz")) ||
                   (fsinfo[fsi].filesystem->exists("/gzipper.js")));

  //GOOD TO HERE

  int linecounter = 0;
  recurseFolder(req, params, path, !sit, maxtiefe, true, linecounter);

  String cache = antworttrenner + "<span title=\"";

  for (uint8_t i = 0; i < maxfilesystem; i++)
  {
    cache += "FS " + String(i) + ": " + fsinfo[i].fsname + "\n";
  }
  cache += "\">&nbsp; Size: " +
           dispFileString(totalBytes(fsinfo[fsi].filesystem), true) +
           ", used: " +
           dispFileString(usedBytes(fsinfo[fsi].filesystem), true) +
           "</span>";

  cache += antworttrenner;
  cache += "<select id=\"memory\" name=\"memory\" onchange=\"fsselectonchange();\">";
  for (int i = 0; i < maxfilesystem; i++)
  {
    if (i == fsi)
    {
      cache += "<option selected";
    }
    else
    {
      cache += "<option";
    }
    cache += ">" + fsinfo[i].fsname + "</option>";
  }
  cache += "</select>";

  cache += "<input type=\"checkbox\" id=\"treeview\" name=\"treeview\" onchange=\"fsselectonchange();\"";
  if (ShowInTreeView(params))
  {
    cache += " checked ";
  }
  cache += "/><label for=\"treeview\">Folders</label>";

  res = httpd_resp_send_chunk(req, cache.c_str(), cache.length());
  // The End.

  //Respond with an empty chunk to signal HTTP response completion
  httpd_resp_send_chunk(req, NULL, 0);

  esphome::esp32_sdmmc::global_ESP32SDMMC->return_sd_lock();

  return res;
}

esp_err_t ESP32SDFM::BootInfo_handler_(struct httpd_req *req) {
  esp_err_t res = ESP_FAIL;

  // hier kann man die globalen Stati initialisieren, weil man weiß, dass die Webseite gerade frisch geladen wird.
  lastFileSystemIndex = -1;

  String fsinfos = "";
  for (uint8_t i = 0; i < maxfilesystem; i++)
  {
    fsinfos += String(i) + ": " + fsinfo[i].fsname + " ";
  }

  String cache =             //
      BackgroundColor +      //
      extrabootinfotrenner + //
      ExtraHTMLfoot +        //
      extrabootinfotrenner + //
      WebPageTitle +         //
      extrabootinfotrenner +
      fsinfos +
      extrabootinfotrenner +
      HtmlIncludes;

  res = httpd_resp_set_type(req,  "text/html");
  if (res != ESP_OK) {
    ESP_LOGW(TAG, "SNAPSHOT: failed to set HTTP response type");
    return res;
  }

  res = httpd_resp_send(req, cache.c_str(), cache.length());

  //no need to send empy chunk to signal end as this codepath only has one send.
  return res;
}

esp_err_t ESP32SDFM::job_handler_(struct httpd_req *req) {
  esp_err_t res = ESP_FAIL;
  queryParams params;

  esphome::esp32_sdmmc::global_ESP32SDMMC->get_sd_lock();

  res = extractQueryParams(req, params);

  if (params.num >= 3)
  { // https://www.youtube.com/watch?v=KSxTxynXiBs
    String jobname = params.job;
    if (jobname == "del")
    {
      String fn = getFileNameFromParam(params, flagCanDelete);
      /** /
      Serial.print("Delete: ");
      Serial.print(fn);
      Serial.println();
      /**/
      if (fn == "")
      {
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "URI is not available");
        return res;
      }
      fsinfo[getFileSystemIndex(params)].filesystem->remove(fn);
      // dummy answer
      res = httpd_resp_set_type(req,  "text/plain");
      if (res != ESP_OK) {
        ESP_LOGW(TAG, "SNAPSHOT: failed to set HTTP response type");
      }
      res = httpd_resp_send_chunk(req, "", 0);
      return res;
      // Raus.
    }
    else if (jobname == "ren")
    {
      String fn = getFileNameFromParam(params, flagCanRename);
      String newfn = params.new_;
      /** /
      Serial.print("Rename: ");
      Serial.print(fn);
      Serial.print(" new: ");
      Serial.print(newfn);
      Serial.println();
      /**/
      if (fn == "")
      {
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "URI is not available");
        return res;
      }
      if (!newfn.startsWith("/"))
      {
        newfn = "/" + newfn;
      }
      int fsi = getFileSystemIndex(params);

      if (pathname(fn) == pathname(newfn))
      {
        if (!fsinfo[fsi].filesystem->rename(fn, newfn))
        {
          Serial.println(F("Rename failed (1)."));
        }
      }
      else
      {
        if (!CopyMoveFile(params, fn, newfn, true))
        {
          Serial.println(F("Rename failed (2)."));
        }
      }
      // dummy answer
      res = httpd_resp_set_type(req,  "text/plain");
      if (res != ESP_OK) {
        ESP_LOGW(TAG, "SNAPSHOT: failed to set HTTP response type");
      }
      res = httpd_resp_send(req, "", 0);
      // Raus.
      return res; //<<==========================
    }
    else if (jobname == "edit")
    {
      String fn = getFileNameFromParam(params, flagCanEdit);
      /** /
      Serial.print("Edit: ");
      Serial.print(fn);
      Serial.println();
      /**/
      if (fn == "")
      {
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "URI is not available");
        return res;
      }
      fileManagerFileEditorInsert(req, params, fn);
      return res; //<<==========================
    }
    else if (jobname == "dwnldll") // downloadall
    {
      String fn = getFileNameFromParam(params, flagAllowInZip | flagCanDownload);
      /** /
      Serial.print("Download: ");
      Serial.print(fn);
      Serial.println();
      /**/
      if (fn == "")
      {
          httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "URI is not available");
         return res;
      }
      return res; //<<==========================
    }
    else if ((jobname == "download") || (jobname == "preview"))
    {
      String fn = getFileNameFromParam(params, flagCanDownload);
      /** /
      Serial.print(F("Download: "));
      Serial.print(fn);
      Serial.println();
      /**/
      if (fn == "")
      {
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "URI is not available");
        return res;
      }
      if (jobname == "download")
      {
        fileManagerDownload(req, params, fn);
      }
      else
      {
        servefile(req, params, fn);
      }
      return res; //<<==========================
    }
    else if (jobname == "createnew")
    {
      String fn = getFileNameFromParam(params, flagCanCreateNew);
      // benötigt einen Filenamen-Fragment als Parameter, <nummer>.txt wird hier angefügt
      /** /
      Serial.print(F("CreateNew: "));
      Serial.print(fn);
      Serial.println();
      /**/
      if (!fn.startsWith("/"))
      {
        fn = "/" + fn;
      }
      if (fn == "")
      {
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "URI is not available");
        return res;
      }
      int fsi = getFileSystemIndex(params);

      int index = 0;
      while (fsinfo[fsi].filesystem->exists(fn + String(index) + ".txt"))
      {
        index++;
      }
      File file = fsinfo[fsi].filesystem->open(fn + String(index) + ".txt", FILE_WRITE);
      file.close();

      res = httpd_resp_set_type(req,  "text/plain");
      if (res != ESP_OK) {
        ESP_LOGW(TAG, "SNAPSHOT: failed to set HTTP response type");
      }
      res = httpd_resp_send(req, "", 0);
      return res; //<<==========================
    }
  }

  // in case all fail, ends here
  httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "URI is not available");

  esphome::esp32_sdmmc::global_ESP32SDMMC->return_sd_lock();

  return res;

}

bool ESP32SDFM::ShowInTreeView(queryParams params)
{
  if (params.num >= 1)
  {
    String tvs = params.t;
    return tvs == "true";
  }
  else
  {
    return false;
  }
}

String ESP32SDFM::CurrentPath(queryParams params)
{
  if (params.num >= 1)
  {
    return params.pn;
  }
  else
  {
    return "";
  }
}

int ESP32SDFM::getFileSystemIndex(queryParams params, bool uselastFileSystemIndex)
{
  if (params.num >= 1)
  {
    String fsis = params.fs;
    int fsi = fsis.toInt();
    if ((fsi >= 0) && (fsi <= sizeof(fsinfo) / sizeof(fsinfo[0])))
    {
      /*
        Serial.print("FS: ");
        Serial.print(fsinfo[fsi].fsname);
        Serial.println();
      */
      lastFileSystemIndex = fsi;
      return fsi;
    }
  }

  if ((uselastFileSystemIndex) && (lastFileSystemIndex != -1))
  {
    return lastFileSystemIndex;
  }

  return 0;
}

esp_err_t ESP32SDFM::extractQueryParams(struct httpd_req *req, queryParams params){
    esp_err_t res = ESP_OK;

    char*  buf;
    size_t buf_len;

    /* Read URL query string length and allocate memory for length + 1,
     * extra byte for null termination */
    buf_len = httpd_req_get_url_query_len(req) + 1;
    if (buf_len > 1) {
        buf = (char*)malloc(buf_len);
        if (buf == nullptr) {
          ESP_LOGE(TAG, "ERR: Failed to allocate snapshot buffer!\n");
          return ESP_FAIL;
        }
        if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK) {
            ESP_LOGI(TAG, "Found URL query => %s", buf);
            char param[EXAMPLE_HTTP_QUERY_KEY_MAX_LEN]= {0};
            char dec_param[EXAMPLE_HTTP_QUERY_KEY_MAX_LEN] = {0};
            /* Get value of expected key from query string */
            if (httpd_query_key_value(buf, "fs", param, sizeof(param)) == ESP_OK) {
                ESP_LOGI(TAG, "Found URL query parameter => fs=%s", param);
                example_uri_decode(dec_param, param, strnlen(param, EXAMPLE_HTTP_QUERY_KEY_MAX_LEN));
                strncpy(params.fs, dec_param, sizeof(dec_param));
                params.num = params.num + 1;
                ESP_LOGI(TAG, "Decoded query parameter => %s", dec_param);
            }
            if (httpd_query_key_value(buf, "fn", param, sizeof(param)) == ESP_OK) {
                ESP_LOGI(TAG, "Found URL query parameter => fn=%s", param);
                example_uri_decode(dec_param, param, strnlen(param, EXAMPLE_HTTP_QUERY_KEY_MAX_LEN));
                strncpy(params.fn, dec_param, sizeof(dec_param));
                params.num = params.num + 1;
                ESP_LOGI(TAG, "Decoded query parameter => %s", dec_param);
            }
            if (httpd_query_key_value(buf, "job", param, sizeof(param)) == ESP_OK) {
                ESP_LOGI(TAG, "Found URL query parameter => job=%s", param);
                example_uri_decode(dec_param, param, strnlen(param, EXAMPLE_HTTP_QUERY_KEY_MAX_LEN));
                strncpy(params.job, dec_param, sizeof(dec_param));
                params.num = params.num + 1;
                ESP_LOGI(TAG, "Decoded query parameter => %s", dec_param);
            }
            if (httpd_query_key_value(buf, "new", param, sizeof(param)) == ESP_OK) {
                ESP_LOGI(TAG, "Found URL query parameter => new=%s", param);
                example_uri_decode(dec_param, param, strnlen(param, EXAMPLE_HTTP_QUERY_KEY_MAX_LEN));
                strncpy(params.new_, dec_param, sizeof(dec_param));
                params.num = params.num + 1;
                ESP_LOGI(TAG, "Decoded query parameter => %s", dec_param);
            }
            if (httpd_query_key_value(buf, "mode", param, sizeof(param)) == ESP_OK) {
                ESP_LOGI(TAG, "Found URL query parameter => mode=%s", param);
                example_uri_decode(dec_param, param, strnlen(param, EXAMPLE_HTTP_QUERY_KEY_MAX_LEN));
                strncpy(params.mode, dec_param, sizeof(dec_param));
                params.num = params.num + 1;
                ESP_LOGI(TAG, "Decoded query parameter => %s", dec_param);
            }
            if (httpd_query_key_value(buf, "folder", param, sizeof(param)) == ESP_OK) {
                ESP_LOGI(TAG, "Found URL query parameter => folder=%s", param);
                example_uri_decode(dec_param, param, strnlen(param, EXAMPLE_HTTP_QUERY_KEY_MAX_LEN));
                strncpy(params.folder, dec_param, sizeof(dec_param));
                params.num = params.num + 1;
                ESP_LOGI(TAG, "Decoded query parameter => %s", dec_param);
            }
            if (httpd_query_key_value(buf, "t", param, sizeof(param)) == ESP_OK) {
                ESP_LOGI(TAG, "Found URL query parameter => t=%s", param);
                example_uri_decode(dec_param, param, strnlen(param, EXAMPLE_HTTP_QUERY_KEY_MAX_LEN));
                strncpy(params.t, dec_param, sizeof(dec_param));
                params.num = params.num + 1;
                ESP_LOGI(TAG, "Decoded query parameter => %s", dec_param);
            }
            if (httpd_query_key_value(buf, "pn", param, sizeof(param)) == ESP_OK) {
                ESP_LOGI(TAG, "Found URL query parameter => pn=%s", param);
                example_uri_decode(dec_param, param, strnlen(param, EXAMPLE_HTTP_QUERY_KEY_MAX_LEN));
                strncpy(params.pn, dec_param, sizeof(dec_param));
                params.num = params.num + 1;
                ESP_LOGI(TAG, "Decoded query parameter => %s", dec_param);
            }
        }
        free(buf);
    }

    // @TODO save parameters as object and return it for other sections to use.
    return res;
}

void ESP32SDFM::recurseFolder(struct httpd_req *req, queryParams params, String foldername, bool flatview, int maxtiefe, bool iststart, int &linecounter)
{
  int fsi = getFileSystemIndex(params, false);
  /** /
  Serial.print("fsi: ");
  Serial.print(fsi);
  Serial.println();
  /**/

  if ((!flatview) && (iststart))
  {
    if (foldername != "/")
    {
      String cache = "-1:..:" + Folder1LevelUp(foldername);
      cache += itemtrenner;
      cache += "-2:" + foldername;
      cache += itemtrenner;
      httpd_resp_send_chunk(req, cache.c_str(), cache.length());

      /** /
      fileManager->sendContent("-1:..:" + Folder1LevelUp(foldername));
      fileManager->sendContent(itemtrenner); // 0
      fileManager->sendContent("-2:" + foldername);
      fileManager->sendContent(itemtrenner); // 1
      /**/
    }
    recurseFolderList(req, params, foldername, -1, 0);
  }

  // Trenner. Beim Start senden
  if (iststart)
  {
    httpd_resp_send_chunk(req, beginoffiles.c_str(), beginoffiles.length());
  }

  //  Schritt 2: die Dateien in dem Ordner oder alles
  File root = fsinfo[fsi].filesystem->open(foldername);
  File file = root.openNextFile();
  while (file)
  {
    if (file.isDirectory())
    {
      if (flatview)
      {
        recurseFolder(req, params, file.path(), flatview, maxtiefe, false, linecounter);
      }
    }
    else
    {
      // Serial.println(file.name());

      /* this is a bad solution, because what if the string changes?
         I couldn't find the source of this string in expressif/esp32/arduino sources.
      */

      if (!((fsinfo[fsi].filesystem == &SD) &&
            (String(file.path()).startsWith(svi))))
      {
        uint32_t flags = ~0;
        if (!gzipperexists)
        {
          flags &= (~ESP32SDFM::flagCanGZip);
        }

        if (!(flags & ESP32SDFM::flagIsNotVisible))
        {
          String cache = String(file.path());
          cache += itemtrenner;
          cache += DeUmlautFilename(String(file.path()));
          cache += itemtrenner;
          cache += dispFileString(file.size(), false);
          cache += itemtrenner;
          cache += colorline(linecounter);
          cache += itemtrenner;
          cache += String(flags);
          cache += itemtrenner;
          httpd_resp_send_chunk(req, cache.c_str(), cache.length());

          /** /
          fileManager->sendContent(String(file.path()));                   // .path() ist fqfn, .name() nur fn?
          fileManager->sendContent(itemtrenner);                           // 0
          fileManager->sendContent(DeUmlautFilename(String(file.path()))); // Display Name
          fileManager->sendContent(itemtrenner);                           // 1
          fileManager->sendContent(dispFileString(file.size(), false));
          fileManager->sendContent(itemtrenner); // 2
          fileManager->sendContent(colorline(linecounter));
          fileManager->sendContent(itemtrenner); // 3
          fileManager->sendContent(String(flags));
          fileManager->sendContent(itemtrenner); // 4
          /**/
          linecounter++;
        }
      }
    }
    file = root.openNextFile();
  }
}

String ESP32SDFM::Folder1LevelUp(String foldername)
{
  /** /
Serial.println(foldername);
  /**/
  int i = foldername.length();
  while ((i > 0) && (foldername.charAt(i) != '/'))
  {
    i--;
  }
  if (i > 0)
  {
    /** /
    Serial.println(foldername.substring(0, i));
    /**/
    return foldername.substring(0, i);
  }
  else
  {
    return "/";
  }
}

String ESP32SDFM::dispFileString(uint64_t fs, bool printorg)
{
  if (fs < 0)
  {
    return "-0";
  }

  if (fs == 0)
  {
    return "0 B";
  }

  if (fs < 1000)
  {
    return String(fs) + " B";
  }

  String units[] = {"B", "kB", "MB", "GB", "TB"}; // Yes, small k, large everything else..., SI-Präfix
  int digitGroups = (int)(log10(fs) / log10(1024));
  if (printorg)
  {
    return String(fs / pow(1024, digitGroups)) + " " + units[digitGroups] + " <small>(" + dispIntDotted(fs) + " B)</small>";
  }
  else
  {
    return String(fs / pow(1024, digitGroups)) + " " + units[digitGroups];
  }
}

void ESP32SDFM::recurseFolderList(struct httpd_req *req, queryParams params, String foldername, int maxtiefe, int tiefe)
{
  int fsi = getFileSystemIndex(params, false);

  // Schritt 1: die Ordner
  File root = fsinfo[fsi].filesystem->open(foldername);
  File file = root.openNextFile();
  while (file)
  {
    if (file.isDirectory())
    {
      if (!(String(file.path()).startsWith(svi)))
      {
        /** /
        Serial.print("Pfad: ");
        Serial.println(String(file.path()));
        Serial.print("Name: ");
        Serial.println(String(file.name()));
        /**/
        uint32_t flags = ~0;
        if (!(flags & ESP32SDFM::flagIsNotVisible))
        {
          String cache = String(tiefe) + ":" + DeUmlautFilename(String(file.path()));
          httpd_resp_send_chunk(req, cache.c_str(), cache.length());
          httpd_resp_send_chunk(req, itemtrenner.c_str(), itemtrenner.length());
        }
      }
      if (tiefe < maxtiefe)
      {
        recurseFolderList(req, params, file.path(), maxtiefe, tiefe + 1);
      }
    }
    file = root.openNextFile();
  }
}

// total/used are not exposed in FS::FS. Who knows why.
uint64_t ESP32SDFM::totalBytes(fs::FS *fs)
{
  if (fs == &SD_MMC)
  {
    return SD_MMC.totalBytes();
  }
  else if (fs == &SD)
  {
    return SD.totalBytes();
  }
  else if (fs == &LittleFS)
  {
    return LittleFS.totalBytes();
  }
  else if (fs == &FFat)
  {
    return FFat.totalBytes();
  }
  else
  {
    return -1;
  }
}

uint64_t ESP32SDFM::usedBytes(fs::FS *fs)
{
  if (fs == &SD_MMC)
  {
    return SD_MMC.usedBytes();
  }
  else if (fs == &SD)
  {
    return SD.usedBytes();
  }
  else if (fs == &LittleFS)
  {
    return LittleFS.usedBytes();
  }
  else if (fs == &FFat)
  {
    return FFat.usedBytes();
  }
  else
  {
    return -1;
  }
}

String ESP32SDFM::CheckFileNameLengthLimit(String fn)
{
  // SPIFFS file name limit. Is there a way to get the max length from SPIFFS/LittleFS?
  //                                      SPIFFS_OBJ_NAME_LEN is spifLittleFS.... but not very clean.
  if (fn.length() > 32)
  {
    int len = fn.length();
    fn.remove(29);
    fn += String(len);
  }

  return fn;
}

String ESP32SDFM::getFileNameFromParam(queryParams params, uint32_t flag)
{
  /* the url looks like
       job?fs=xx&fn=filename&job=jobtoken
     plus extra params for some jobs.
  */
  /** /
  Serial.println("Params");
  for (int i = 0; i < fileManager->args(); i++)
  {
    Serial.print(fileManager->argName(i));
    Serial.print("=");
    Serial.print(fileManager->arg(i));
    Serial.println();
  }
  /**/

  if (params.num < 3)
  {
    /** /
    Serial.println("Args < 3");
    /**/
    return "";
  }

  String fn = params.fn;

  if (fn == "")
  {
    /** /
    Serial.println("arg(fn) is empty");
    /**/
    return "";
  }

  int fsi = getFileSystemIndex(params);

  // Sonderregel, wenn eine neue Datei erstellt werden soll
  if ((flag & flagCanCreateNew) || (flag & flagAllowInZip))
  {
    return fn;
  }
  else
  {
    if (fsinfo[fsi].filesystem->exists(fn))
    { // file exists!
      // Yeah.
      return fn;
    }
  }

  /** /
Serial.println("Return nothing");
  /**/
  return "";
}

bool ESP32SDFM::CopyMoveFile(queryParams params, String oldfilename, String newfilename, bool move)
{
  // Zusammensuchen der FilesystemIndizes
  int fsiofn = getFSidxfromFilename(oldfilename);
  int fsinfn = getFSidxfromFilename(newfilename);

  if (fsiofn == -1)
  {
    fsiofn = getFileSystemIndex(params);
  }
  if (fsinfn == -1)
  {
    fsinfn = fsiofn;
  }

  // Aufräumen der Dateinamen
  oldfilename = getCleanFilename(oldfilename);
  newfilename = getCleanFilename(newfilename);

  // Neuen Ordner bauen, vorsichtshalber. Stückweise.
  int i = 1;
  String pn = pathname(newfilename);
  while (i < pn.length())
  {
    if (pn.charAt(i) == '/')
    {
      fsinfo[fsinfn].filesystem->mkdir(pn.substring(0, i));
    }
    i++;
  }
  fsinfo[fsinfn].filesystem->mkdir(pn);

  File oldfile = fsinfo[fsiofn].filesystem->open(oldfilename, FILE_READ);
  File newfile = fsinfo[fsinfn].filesystem->open(newfilename, FILE_WRITE);

  if ((oldfile) && (newfile))
  {
    const int bufsize = 4 * 1024;
    uint8_t *buffer;
    buffer = new uint8_t[4 * 1024];

    int bytesread = 0;
    int byteswritten = 0;
    while (oldfile.available())
    {
      size_t r = oldfile.read(buffer, bufsize);
      bytesread += r;
      byteswritten += newfile.write(buffer, r);
    }

    delete[] buffer;

    oldfile.close();
    newfile.close();

    // remove only, if new file is fully written.
    if ((move) && (bytesread == byteswritten))
    {
      fsinfo[fsiofn].filesystem->remove(oldfilename);
    }

    return true;
  }
  else
  {
    if (oldfile)
    {
      Serial.println(F("CMF: newfile fail."));
    }
    else
    {
      Serial.println(F("CMF: oldfile fail."));
    }
    return false;
  }
}

String ESP32SDFM::DeUmlautFilename(String fn)
{ // cp437/cp850 to ...
  String nfn = "";
  for (int i = 0; i < fn.length(); i++)
  {
    switch (fn[i])
    {
    case 0x84:
      nfn += "\u00e4";
      break;
    case 0x94:
      nfn += "\u00f6";
      break;
    case 0x81:
      nfn += "\u00fc";
      break;
    case 0x8E:
      nfn += "\u00c4";
      break;
    case 0x99:
      nfn += "\u00d6";
      break;
    case 0x9A:
      nfn += "\u00dc";
      break;
    case 0xE1:
      nfn += "\u00df";
      break;
      // €	\u20ac

    default:
      nfn += fn[i];
      break;
    }
  }

  return nfn;
}

String ESP32SDFM::dispIntDotted(size_t i)
{
  String res = "";
  while (i != 0)
  {
    int r = i % 1000;
    res = String(i % 1000) + res;
    i /= 1000;
    if ((r < 100) && (i > 0))
    {
      res = "0" + res;
      if (r < 10)
      {
        res = "0" + res;
      }
    }
    if (i != 0)
    {
      res = "." + res;
    }
  }
  return res;
}

String ESP32SDFM::colorline(int i)
{
  if (i % 2 == 0)
  {
    return "ccu";
  }
  else
  {
    return "ccg";
  }
}

void ESP32SDFM::fileManagerFileEditorInsert(struct httpd_req *req, queryParams params,  String &filename)
{
  esp_err_t res = ESP_OK;

  // Serial.println("Edit");
  res = httpd_resp_set_type(req,  "text/html");
  if (res != ESP_OK) {
    ESP_LOGW(TAG, "SNAPSHOT: failed to set HTTP response type");
  }
  httpd_resp_send_chunk(req, ESPFMfGKWpFormIntro1, strlen(ESPFMfGKWpFormIntro1));
  httpd_resp_send_chunk(req, textareaCharset.c_str(), textareaCharset.length());
  httpd_resp_send_chunk(req, ESPFMfGKWpFormIntro2, strlen(ESPFMfGKWpFormIntro2));

  httpd_resp_set_type(req, "text/text");
  if (fsinfo[getFileSystemIndex(params)].filesystem->exists(filename))
  {
    File f = fsinfo[getFileSystemIndex(params)].filesystem->open(filename, "r");
    if (f)
    {
      const int chuncksize = 2 * 1024;
      String cache = "";
      do
      {
        cache += f.readStringUntil('\n') + '\n';
        if (cache.length() >= chuncksize)
        {
          cache = escapeHTMLcontent(cache);
          httpd_resp_send_chunk(req, cache.c_str(), cache.length());
          cache = "";
        }
      } while (f.available());
      f.close();
      if (cache != "")
      {
        httpd_resp_send_chunk(req, cache.c_str(), cache.length());
      }
    }
  }
  else
  {
    /** /
    Serial.println(filename);
    Serial.println(F("File not found"));
    /**/
  }

  httpd_resp_send_chunk(req, ESPFMfGKWpFormExtro1, strlen(ESPFMfGKWpFormExtro1));
  httpd_resp_send_chunk(req, "", 0);
}

void ESP32SDFM::fileManagerDownload(struct httpd_req *req, queryParams params, String &filename)
{
  esp_err_t res = ESP_OK;

  // filesystem set by caller
  File f = fsinfo[getFileSystemIndex(params)].filesystem->open(filename, "r");
  if (f)
  {
    // ohne führend slash
    if (filename.startsWith("/"))
    {
      filename.remove(0, 1);
    }


    ESP_LOGI(TAG, "Sending file : %s ...", filename);
    httpd_resp_set_type(req, getContentType(filename).c_str());

    /* Retrieve the pointer to scratch buffer for temporary storage */
    char *chunk = ((struct file_server_data *)req->user_ctx)->scratch;
    size_t chunksize;
    do {
        /* Read file in chunks into the scratch buffer */
        chunksize = f.readBytes(chunk, SCRATCH_BUFSIZE);

        if (chunksize > 0) {
            /* Send the buffer contents as HTTP response chunk */
            if (httpd_resp_send_chunk(req, chunk, chunksize) != ESP_OK) {
                f.close();
                ESP_LOGE(TAG, "File sending failed!");
                /* Abort sending file */
                httpd_resp_sendstr_chunk(req, NULL);
                /* Respond with 500 Internal Server Error */
                httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to send file");
               return;
           }
        }

        /* Keep looping till the whole file is sent */
    } while (chunksize != 0);

    f.close();
    ESP_LOGI(TAG, "File sending complete");
    httpd_resp_send_chunk(req, NULL, 0);
    return;
  }
}

void ESP32SDFM::servefile(struct httpd_req *req, queryParams params, String uri)
{
  // Handle the servinf of the "fm.*"-files
  int fsi = getFileSystemIndex(params, false);
  if (fsi == -1)
  {
    fsi = 0;
  }
  /** /
  Serial.print(F("File system id: "));
  Serial.println(fsi);
  /**/

  if (fsinfo[fsi].filesystem->exists(uri))
  {
    File f = fsinfo[fsi].filesystem->open(uri, "r");
    if (f)
    {
    ESP_LOGI(TAG, "Sending file : %s ...", uri);
    httpd_resp_set_type(req, getContentType(uri).c_str());

    /* Retrieve the pointer to scratch buffer for temporary storage */
    char *chunk = ((struct file_server_data *)req->user_ctx)->scratch;
    size_t chunksize;
    do {
        /* Read file in chunks into the scratch buffer */
        chunksize = f.readBytes(chunk, SCRATCH_BUFSIZE);

        if (chunksize > 0) {
            /* Send the buffer contents as HTTP response chunk */
            if (httpd_resp_send_chunk(req, chunk, chunksize) != ESP_OK) {
                f.close();
                ESP_LOGE(TAG, "File sending failed!");
                /* Abort sending file */
                httpd_resp_sendstr_chunk(req, NULL);
                /* Respond with 500 Internal Server Error */
                httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to send file");
               return;
           }
        }

        /* Keep looping till the whole file is sent */
    } while (chunksize != 0);

    f.close();
    ESP_LOGI(TAG, "File sending complete");
    httpd_resp_send_chunk(req, NULL, 0);
    }
  }

  httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "URI is not available");
}

int ESP32SDFM::getFSidxfromFilename(String fn)
{
  int i = fn.indexOf(":");
  if (i > -1)
  {
    fn = fn.substring(0, i - 1);
    int fnidx = fn.toInt();
    // Limits
    if ((fnidx < 0) || (fnidx >= maxfilesystem))
    {
      return -1;
    }
    else
    {
      return fnidx;
    }
  }
  else
  {
    return -1;
  }
}

String ESP32SDFM::getCleanFilename(String fn)
{
  int i = fn.indexOf(":");
  if (i > -1)
  {
    return fn.substring(i + 1, fn.length() - 2);
  }
  else
  {
    return fn;
  }
}

String ESP32SDFM::pathname(String fn)
{
  // find last "/"
  int i = fn.lastIndexOf("/");

  if (i > -1)
  {
    // Serial.println(fn.substring(0, i));
    return fn.substring(0, i);
  }
  else
  {
    return "/";
  }
}

String ESP32SDFM::escapeHTMLcontent(String html)
{
  // html.replace("<","&lt;");
  // html.replace(">","&gt;");
  html.replace("&", "&amp;");

  return html;
}

String ESP32SDFM::getContentType(const String &path)
{
  String dataType = "text/plain";
  if (path.endsWith(".htm")) {
    dataType = "text/html";
  } else if (path.endsWith(".css")) {
    dataType = "text/css";
  } else if (path.endsWith(".js")) {
    dataType = "application/javascript";
  } else if (path.endsWith(".png")) {
    dataType = "image/png";
  } else if (path.endsWith(".gif")) {
    dataType = "image/gif";
  } else if (path.endsWith(".jpg")) {
    dataType = "image/jpeg";
  } else if (path.endsWith(".ico")) {
    dataType = "image/x-icon";
  } else if (path.endsWith(".xml")) {
    dataType = "text/xml";
  } else if (path.endsWith(".pdf")) {
    dataType = "application/pdf";
  } else if (path.endsWith(".zip")) {
    dataType = "application/zip";
  }
  return dataType;
}

} //namespace esp32_sd_filemanager
} //namespace esphome