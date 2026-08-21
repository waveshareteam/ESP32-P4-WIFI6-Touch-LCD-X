/*
 * SPDX-FileCopyrightText: 2026 Waveshare
 * SPDX-License-Identifier: Apache-2.0
 *
 * microSD card read/write demo for the Waveshare ESP32-P4-WIFI6-Touch-LCD-X.
 *
 * The microSD slot is wired to the SDIO 3.0 interface:
 *   CLK=GPIO43, CMD=GPIO44, D0=GPIO39, D1=GPIO40, D2=GPIO41, D3=GPIO42
 *
 * The sketch only creates and reads its own demo file; it does not remove or
 * modify other files on the card.
 */
#include <Arduino.h>
#include <FS.h>
#include <SD_MMC.h>

namespace {

constexpr int kSdClk = 43;
constexpr int kSdCmd = 44;
constexpr int kSdD0 = 39;
constexpr int kSdD1 = 40;
constexpr int kSdD2 = 41;
constexpr int kSdD3 = 42;
constexpr char kDemoPath[] = "/waveshare_lcd_x_sd_demo.txt";

const char *card_type_name(uint8_t card_type)
{
  switch (card_type) {
    case CARD_MMC:
      return "MMC";
    case CARD_SD:
      return "SDSC";
    case CARD_SDHC:
      return "SDHC";
    default:
      return "UNKNOWN";
  }
}

bool write_demo_file()
{
  File file = SD_MMC.open(kDemoPath, FILE_WRITE);
  if (!file) {
    Serial.printf("Failed to open %s for writing\n", kDemoPath);
    return false;
  }

  if (!file.println("Hello from ESP32-P4-WIFI6-Touch-LCD-X microSD demo!")) {
    Serial.printf("Failed to write %s\n", kDemoPath);
    file.close();
    return false;
  }
  file.close();
  Serial.printf("Appended to %s\n", kDemoPath);
  return true;
}

bool read_demo_file()
{
  File file = SD_MMC.open(kDemoPath, FILE_READ);
  if (!file) {
    Serial.printf("Failed to open %s for reading\n", kDemoPath);
    return false;
  }

  Serial.printf("Read back %s:\n", kDemoPath);
  while (file.available()) {
    Serial.write(file.read());
  }
  file.close();
  return true;
}

void list_root_directory()
{
  File root = SD_MMC.open("/");
  if (!root || !root.isDirectory()) {
    Serial.println("Failed to open SD card root directory");
    return;
  }

  Serial.println("Root directory:");
  File entry = root.openNextFile();
  while (entry) {
    Serial.printf("  %s (%u bytes)\n", entry.name(), static_cast<unsigned>(entry.size()));
    entry.close();
    entry = root.openNextFile();
  }
  root.close();
}

}  // namespace

void setup()
{
  Serial.begin(115200);
  delay(500);

  if (!SD_MMC.setPins(kSdClk, kSdCmd, kSdD0, kSdD1, kSdD2, kSdD3)) {
    Serial.println("SD_MMC pin configuration failed");
    return;
  }
  if (!SD_MMC.begin("/sdcard", false /* 4-bit bus */)) {
    Serial.println("Card mount failed; insert a FAT32-formatted microSD card");
    return;
  }

  const uint8_t card_type = SD_MMC.cardType();
  if (card_type == CARD_NONE) {
    Serial.println("No microSD card detected");
    SD_MMC.end();
    return;
  }

  Serial.printf("Card type: %s\n", card_type_name(card_type));
  Serial.printf("Card size: %llu MB\n", SD_MMC.cardSize() / (1024ULL * 1024ULL));
  Serial.printf("Total space: %llu MB\n", SD_MMC.totalBytes() / (1024ULL * 1024ULL));
  Serial.printf("Used space: %llu MB\n", SD_MMC.usedBytes() / (1024ULL * 1024ULL));

  if (!write_demo_file() || !read_demo_file()) {
    SD_MMC.end();
    return;
  }
  list_root_directory();
  Serial.println("SD card demo finished");
}

void loop()
{
  delay(1000);
}
