#pragma once

#include <cinttypes>
#include <freertos/FreeRTOS.h>

#include <Arduino.h>

#include "esphome/core/component.h"
#include "esphome/core/helpers.h"
#include "esphome/core/preferences.h"

#include "FS.h"

struct httpd_req;

namespace esphome {
namespace esp32_sd_filemanager {

#define EXAMPLE_HTTP_QUERY_KEY_MAX_LEN  (64)
/* Scratch buffer size */
#define SCRATCH_BUFSIZE  8192

struct queryParams {
    char fs[EXAMPLE_HTTP_QUERY_KEY_MAX_LEN]= {0};
    char fn[EXAMPLE_HTTP_QUERY_KEY_MAX_LEN]= {0};
    char job[EXAMPLE_HTTP_QUERY_KEY_MAX_LEN]= {0};
    char new_[EXAMPLE_HTTP_QUERY_KEY_MAX_LEN]= {0};
    char folder[EXAMPLE_HTTP_QUERY_KEY_MAX_LEN]= {0};
    char mode[EXAMPLE_HTTP_QUERY_KEY_MAX_LEN]= {0};
    char t[EXAMPLE_HTTP_QUERY_KEY_MAX_LEN]= {0};
    char pn[EXAMPLE_HTTP_QUERY_KEY_MAX_LEN]= {0};
    int num = 0;
};

struct file_server_data {
    /* Base path of file storage */
    char base_path[64];

    /* Scratch buffer for temporary storage during file transfer */
    char scratch[SCRATCH_BUFSIZE];
};

class ESP32SDFM : public Component {
 public:
  ESP32SDFM();
  ~ESP32SDFM();

  void setup() override;
  void dump_config() override;
  float get_setup_priority() const override;
  void set_port(uint16_t port) { this->port_ = port; }
  void loop() override;

  // must be a valid css color name, see https://en.wikipedia.org/wiki/Web_colors
  String BackgroundColor = "";
  // additional html inserted into the foot below the web page
  String ExtraHTMLfoot = "";
  String WebPageTitle = "";
  // set "accept-charset=\"utf-8\"" for utf-8 support in textarea
  String textareaCharset = "";
  // add this html-files as windowed item, can be list ;-separated
  String HtmlIncludes = "";

// Flags, sync with fm.js, this has some room to grow, 32 bits ought to be enough for anybody
  const static uint32_t flagCanDelete = 1 << 0;
  const static uint32_t flagCanRename = 1 << 1;
  // see CanUpload, Edits "save" button will fail if not set
  const static uint32_t flagCanEdit = 1 << 2;
  // Allowed to be previewed. Browser does the preview, so it depends on that.
  const static uint32_t flagAllowPreview = 1 << 3;
  const static uint32_t flagCanGZip = 1 << 4;
  const static uint32_t flagCanDownload = 1 << 5;
  const static uint32_t flagAllowInZip = 1 << 6;
  // A file with this name can be uploaded. An upload wont work if this flag is not set!
  const static uint32_t flagCanUpload = 1 << 7;
  // File will not be shown at all
  const static uint32_t flagIsNotVisible = 1 << 8;
  // beim Umbenennen
  const static uint32_t flagIsValidTargetFilename =  1 << 9;
  // beim Überprüfen, ob eine Dateisystem-Aktion zulässig ist
  const static uint32_t flagIsValidAction =  1 << 10;
  // allowed to create new files
  const static uint32_t flagCanCreateNew =  1 << 11;



 protected:
  esp_err_t index_handler_(struct httpd_req *req);
  esp_err_t CSS_handler_(struct httpd_req *req);
  esp_err_t JS_handler_(struct httpd_req *req);
  esp_err_t FileListInsert_handler_(struct httpd_req *req);
  esp_err_t BootInfo_handler_(struct httpd_req *req);
  esp_err_t job_handler_(struct httpd_req *req);
  esp_err_t HtmlIncludesInterface_handler_(struct httpd_req *req);
  esp_err_t ReceiverOK_handler_(struct httpd_req *req);

  bool AddFS(fs::FS &fs, String FSname, bool AutoTreemode);

  int getFileSystemIndex(queryParams params, bool uselastFileSystemIndex = true);
  String CurrentPath(queryParams params);
  bool ShowInTreeView(queryParams params);
  String Folder1LevelUp(String foldername);
  void recurseFolder(struct httpd_req *req, queryParams params, String foldername, bool flatview, int maxtiefe, bool iststart, int &linecounter);
  void recurseFolderList(struct httpd_req *req, queryParams params, String foldername, int maxtiefe, int tiefe);
  String dispFileString(uint64_t fs, bool printorg);
  String colorline(int i);
  String CheckFileNameLengthLimit(String fn);
  String DeUmlautFilename(String fn);
  String dispIntDotted(size_t i);
  String getFileNameFromParam(queryParams params, uint32_t flag);
  String pathname(String fn);
  bool CopyMoveFile(queryParams params, String oldfilename, String newfilename, bool move);
  void fileManagerFileEditorInsert(struct httpd_req *req, queryParams params,  String &filename);
  void fileManagerDownload(struct httpd_req *req, queryParams params, String &filename);
  void servefile(struct httpd_req *req, queryParams params, String uri);
  int getFSidxfromFilename(String fn);
  String getCleanFilename(String fn);
  String escapeHTMLcontent(String html);
  String getContentType(const String &path);

  esp_err_t extractQueryParams(struct httpd_req *req, queryParams params);

  uint16_t port_{0};
  void *httpd_{nullptr};
  bool gzipperexists;

 private:
  struct FileSystemInfo_t  // sizeof: 24, packed: 21
  {
    String fsname;
    bool AutoTreemode;
    fs::FS *filesystem;
  };

  static const int8_t maxfilesystems = 4;                             // !!!!!!!
  FileSystemInfo_t fsinfo[maxfilesystems];
  int maxfilesystem = 0;
  int lastFileSystemIndex = -1;

  uint64_t totalBytes(fs::FS *fs);
  uint64_t usedBytes(fs::FS *fs);

  // Flags für Datenkommunikation
  String itemtrenner = "\x02\x01\x04";
  String beginoffiles  = "\x03\x01\x02";
  String antworttrenner = "\x02\x01\x03";
  String extrabootinfotrenner = "\x02\x01\x07";

  String svi = "/System Volume Information";
};

} //namespace esp32_sd_filemanager
} //namespace esphome