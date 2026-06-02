#include <Arduboy2.h>
#include <ArduboyTones.h>
#include <EEPROM.h>

Arduboy2 arduboy;
ArduboyTones sound(arduboy.audio.enabled);

constexpr uint8_t ScreenWidth = 128;
constexpr uint8_t ScreenHeight = 64;
constexpr uint8_t HistorySize = 16;
constexpr uint8_t ChargeFrames = 30;
constexpr uint8_t SuccessFrames = 45;
constexpr uint8_t SecretCount = 1;
constexpr uint16_t SaveMagic = 0xD0A0;
constexpr int SaveAddress = 64;

enum InputCode : uint8_t {
  InputUp,
  InputDown,
  InputLeft,
  InputRight,
  InputDownRight,
  InputDownLeft,
  InputA,
  InputB
};

enum ScreenMode : uint8_t {
  ScreenMainMenu,
  ScreenCharacterSelect,
  ScreenMoveSelect,
  ScreenFreePractice,
  ScreenChallenge,
  ScreenSecrets,
  ScreenSuccess
};

enum CharacterId : uint8_t {
  CharacterRyu,
  CharacterKen,
  CharacterGuile,
  CharacterCount
};

enum MoveId : uint8_t {
  MoveHadouken,
  MoveShoryuken,
  MoveTatsumaki,
  MoveSonicBoom,
  MoveFlashKick,
  MoveCount
};

struct Move {
  const char *name;
  const InputCode *sequence;
  uint8_t length;
  CharacterId character;
  bool needsLeftCharge;
  bool needsDownCharge;
};

struct SaveData {
  uint16_t magic;
  uint16_t movesDiscovered;
  uint8_t secretsFound;
  uint8_t highScore;
};

const char *CharacterNames[CharacterCount] = {
  "RYU",
  "KEN",
  "GUILE"
};

const InputCode HadoukenSeq[] = { InputDown, InputDownRight, InputRight, InputA };
const InputCode ShoryukenSeq[] = { InputRight, InputDown, InputDownRight, InputA };
const InputCode TatsumakiSeq[] = { InputDown, InputDownLeft, InputLeft, InputB };
const InputCode SonicBoomSeq[] = { InputRight, InputA };
const InputCode FlashKickSeq[] = { InputUp, InputB };
const InputCode BossSecretSeq[] = {
  InputUp, InputUp, InputDown, InputDown, InputLeft,
  InputRight, InputLeft, InputRight, InputA, InputB
};

const Move Moves[MoveCount] = {
  { "HADOUKEN", HadoukenSeq, 4, CharacterRyu, false, false },
  { "SHORYUKEN", ShoryukenSeq, 4, CharacterRyu, false, false },
  { "TATSUMAKI", TatsumakiSeq, 4, CharacterRyu, false, false },
  { "SONIC BOOM", SonicBoomSeq, 2, CharacterGuile, true, false },
  { "FLASH KICK", FlashKickSeq, 2, CharacterGuile, false, true }
};

InputCode inputHistory[HistorySize];
uint8_t inputCount = 0;
uint8_t menuIndex = 0;
uint8_t selectedCharacter = CharacterRyu;
uint8_t selectedMove = MoveHadouken;
uint8_t challengeMove = MoveHadouken;
uint8_t score = 0;
uint8_t successTimer = 0;
uint8_t leftCharge = 0;
uint8_t downCharge = 0;
bool leftChargeReady = false;
bool downChargeReady = false;
bool bossUnlocked = false;
ScreenMode screen = ScreenMainMenu;
ScreenMode returnScreen = ScreenMainMenu;
SaveData saveData;
char successText[14];

void setup() {
  arduboy.begin();
  arduboy.setFrameRate(30);
  arduboy.initRandomSeed();
  loadSave();
}

void loop() {
  if (!arduboy.nextFrame()) {
    return;
  }

  arduboy.pollButtons();
  updateCharge();

  switch (screen) {
    case ScreenMainMenu:
      updateMainMenu();
      drawMainMenu();
      break;
    case ScreenCharacterSelect:
      updateCharacterSelect();
      drawCharacterSelect();
      break;
    case ScreenMoveSelect:
      updateMoveSelect();
      drawMoveSelect();
      break;
    case ScreenFreePractice:
      updateTraining(false);
      drawTraining(false);
      break;
    case ScreenChallenge:
      updateTraining(true);
      drawTraining(true);
      break;
    case ScreenSecrets:
      updateSecrets();
      drawSecrets();
      break;
    case ScreenSuccess:
      updateSuccess();
      drawSuccess();
      break;
  }
}

void loadSave() {
  EEPROM.get(SaveAddress, saveData);
  if (saveData.magic != SaveMagic) {
    saveData.magic = SaveMagic;
    saveData.movesDiscovered = 0;
    saveData.secretsFound = 0;
    saveData.highScore = 0;
    EEPROM.put(SaveAddress, saveData);
  }

  bossUnlocked = (saveData.secretsFound & 0x01) != 0;
}

void saveProgress() {
  EEPROM.put(SaveAddress, saveData);
}

void updateMainMenu() {
  updateMenuIndex(3);

  if (arduboy.justPressed(A_BUTTON)) {
    if (menuIndex == 0) {
      selectedCharacter = CharacterRyu;
      menuIndex = 0;
      screen = ScreenCharacterSelect;
    } else if (menuIndex == 1) {
      startChallenge();
    } else {
      screen = ScreenSecrets;
    }
  }
}

void updateCharacterSelect() {
  updateMenuIndex(CharacterCount);

  if (arduboy.justPressed(A_BUTTON)) {
    selectedCharacter = menuIndex;
    selectedMove = firstMoveForCharacter(selectedCharacter);
    menuIndex = 0;
    screen = ScreenMoveSelect;
  }

  if (arduboy.justPressed(B_BUTTON)) {
    menuIndex = 0;
    screen = ScreenMainMenu;
  }
}

void updateMoveSelect() {
  uint8_t count = moveCountForCharacter(selectedCharacter);
  updateMenuIndex(count);

  if (arduboy.justPressed(A_BUTTON)) {
    selectedMove = moveForCharacterAt(selectedCharacter, menuIndex);
    clearInputs();
    screen = ScreenFreePractice;
  }

  if (arduboy.justPressed(B_BUTTON)) {
    menuIndex = selectedCharacter;
    screen = ScreenCharacterSelect;
  }
}

void updateSecrets() {
  if (arduboy.justPressed(B_BUTTON | A_BUTTON)) {
    menuIndex = 0;
    screen = ScreenMainMenu;
  }
}

void updateSuccess() {
  if (successTimer < SuccessFrames) {
    successTimer++;
    return;
  }

  clearInputs();
  if (returnScreen == ScreenChallenge) {
    nextChallenge();
  }
  screen = returnScreen;
}

void updateTraining(bool challenge) {
  if (arduboy.pressed(A_BUTTON) && arduboy.justPressed(B_BUTTON)) {
    clearInputs();
    screen = ScreenMainMenu;
    menuIndex = 0;
    return;
  }

  readInputs();
  checkSecrets();

  uint8_t moveId = challenge ? challengeMove : selectedMove;
  if (moveMatches(Moves[moveId])) {
    markMoveDiscovered(moveId);
    if (challenge) {
      addScore();
    }
    showSuccess(Moves[moveId].name, challenge ? ScreenChallenge : ScreenFreePractice);
  }
}

void updateMenuIndex(uint8_t itemCount) {
  if (arduboy.justPressed(UP_BUTTON) && menuIndex > 0) {
    menuIndex--;
  }

  if (arduboy.justPressed(DOWN_BUTTON) && menuIndex + 1 < itemCount) {
    menuIndex++;
  }
}

void startChallenge() {
  score = 0;
  clearInputs();
  nextChallenge();
  screen = ScreenChallenge;
}

void nextChallenge() {
  challengeMove = random(MoveCount);
}

void addScore() {
  if (score < 255) {
    score++;
  }

  if (score > saveData.highScore) {
    saveData.highScore = score;
    saveProgress();
  }
}

void markMoveDiscovered(uint8_t moveId) {
  uint16_t bit = 1 << moveId;
  if ((saveData.movesDiscovered & bit) == 0) {
    saveData.movesDiscovered |= bit;
    saveProgress();
  }
}

void showSuccess(const char *text, ScreenMode nextScreen) {
  strncpy(successText, text, sizeof(successText) - 2);
  successText[sizeof(successText) - 2] = '!';
  successText[sizeof(successText) - 1] = '\0';
  successTimer = 0;
  returnScreen = nextScreen;
  playMoveTone();
  screen = ScreenSuccess;
}

void readInputs() {
  if ((arduboy.justPressed(DOWN_BUTTON) && arduboy.pressed(RIGHT_BUTTON)) ||
      (arduboy.justPressed(RIGHT_BUTTON) && arduboy.pressed(DOWN_BUTTON))) {
    addInput(InputDownRight);
    return;
  }

  if ((arduboy.justPressed(DOWN_BUTTON) && arduboy.pressed(LEFT_BUTTON)) ||
      (arduboy.justPressed(LEFT_BUTTON) && arduboy.pressed(DOWN_BUTTON))) {
    addInput(InputDownLeft);
    return;
  }

  if (arduboy.justPressed(UP_BUTTON)) {
    addInput(InputUp);
  }
  if (arduboy.justPressed(DOWN_BUTTON)) {
    addInput(InputDown);
  }
  if (arduboy.justPressed(LEFT_BUTTON)) {
    addInput(InputLeft);
  }
  if (arduboy.justPressed(RIGHT_BUTTON)) {
    addInput(InputRight);
  }
  if (arduboy.justPressed(A_BUTTON)) {
    addInput(InputA);
  }
  if (arduboy.justPressed(B_BUTTON)) {
    addInput(InputB);
  }
}

void updateCharge() {
  if (arduboy.pressed(LEFT_BUTTON)) {
    if (leftCharge < ChargeFrames) {
      leftCharge++;
    }
    if (leftCharge >= ChargeFrames) {
      leftChargeReady = true;
    }
  } else if (!leftChargeReady) {
    leftCharge = 0;
  }

  if (arduboy.pressed(DOWN_BUTTON)) {
    if (downCharge < ChargeFrames) {
      downCharge++;
    }
    if (downCharge >= ChargeFrames) {
      downChargeReady = true;
    }
  } else if (!downChargeReady) {
    downCharge = 0;
  }
}

void addInput(InputCode input) {
  if (inputCount < HistorySize) {
    inputHistory[inputCount] = input;
    inputCount++;
  } else {
    for (uint8_t i = 1; i < HistorySize; i++) {
      inputHistory[i - 1] = inputHistory[i];
    }
    inputHistory[HistorySize - 1] = input;
  }
}

void clearInputs() {
  inputCount = 0;
  leftCharge = 0;
  downCharge = 0;
  leftChargeReady = false;
  downChargeReady = false;
}

bool moveMatches(const Move &move) {
  if (move.needsLeftCharge && !leftChargeReady) {
    return false;
  }

  if (move.needsDownCharge && !downChargeReady) {
    return false;
  }

  if (!sequenceMatches(move.sequence, move.length)) {
    return false;
  }

  return true;
}

bool sequenceMatches(const InputCode *sequence, uint8_t length) {
  if (inputCount < length) {
    return false;
  }

  uint8_t start = inputCount - length;
  for (uint8_t i = 0; i < length; i++) {
    if (inputHistory[start + i] != sequence[i]) {
      return false;
    }
  }

  return true;
}

void checkSecrets() {
  if (!bossUnlocked && sequenceMatches(BossSecretSeq, 10)) {
    bossUnlocked = true;
    saveData.secretsFound |= 0x01;
    saveProgress();
    showSuccess("BOSS OPEN", screen);
  }
}

uint8_t firstMoveForCharacter(uint8_t character) {
  for (uint8_t i = 0; i < MoveCount; i++) {
    if (Moves[i].character == character || character == CharacterKen) {
      if (character == CharacterGuile && Moves[i].character != CharacterGuile) {
        continue;
      }
      if (character != CharacterGuile && Moves[i].character == CharacterGuile) {
        continue;
      }
      return i;
    }
  }

  return MoveHadouken;
}

uint8_t moveCountForCharacter(uint8_t character) {
  return character == CharacterGuile ? 2 : 3;
}

uint8_t moveForCharacterAt(uint8_t character, uint8_t index) {
  if (character == CharacterGuile) {
    return MoveSonicBoom + index;
  }

  return index;
}

void playMoveTone() {
  sound.tone(440, 45, 660, 55, 880, 80);
}

void drawMainMenu() {
  arduboy.clear();
  arduboy.setCursor(12, 4);
  arduboy.print(F("STREET FIGHT"));
  arduboy.setCursor(42, 14);
  arduboy.print(F("DOJO"));
  drawMenuItem(0, 28, F("FREE PRACTICE"));
  drawMenuItem(1, 38, F("DOJO CHALLENGE"));
  drawMenuItem(2, 48, F("SECRETS"));
  drawFooter();
  arduboy.display();
}

void drawCharacterSelect() {
  arduboy.clear();
  arduboy.setCursor(18, 4);
  arduboy.print(F("CHOOSE FIGHTER"));
  for (uint8_t i = 0; i < CharacterCount; i++) {
    drawMenuItem(i, 22 + i * 10, CharacterNames[i]);
  }
  arduboy.display();
}

void drawMoveSelect() {
  arduboy.clear();
  arduboy.setCursor(20, 4);
  arduboy.print(CharacterNames[selectedCharacter]);
  arduboy.print(F(" TRAINING"));

  uint8_t count = moveCountForCharacter(selectedCharacter);
  for (uint8_t i = 0; i < count; i++) {
    drawMenuItem(i, 22 + i * 10, Moves[moveForCharacterAt(selectedCharacter, i)].name);
  }
  arduboy.display();
}

void drawTraining(bool challenge) {
  uint8_t moveId = challenge ? challengeMove : selectedMove;

  arduboy.clear();
  arduboy.setCursor(0, 0);
  arduboy.print(challenge ? F("DOJO CHALLENGE") : F("FREE PRACTICE"));

  arduboy.setCursor(0, 12);
  arduboy.print(F("Perform:"));
  arduboy.setCursor(0, 22);
  arduboy.print(Moves[moveId].name);
  drawMoveSequence(Moves[moveId], 0, 34);

  drawHistory(0, 48);

  if (challenge) {
    arduboy.setCursor(0, 56);
    arduboy.print(F("Score "));
    arduboy.print(score);
    arduboy.setCursor(76, 56);
    arduboy.print(F("Hi "));
    arduboy.print(saveData.highScore);
  } else {
    drawChargeStatus();
  }

  arduboy.display();
}

void drawSuccess() {
  arduboy.clear();
  uint8_t x = centeredX(successText);
  arduboy.setCursor(x, 26);
  arduboy.print(successText);
  arduboy.display();
}

void drawSecrets() {
  arduboy.clear();
  arduboy.setCursor(34, 4);
  arduboy.print(F("SECRETS"));
  arduboy.setCursor(0, 22);
  arduboy.print(F("Found "));
  arduboy.print(countSecrets());
  arduboy.print(F("/"));
  arduboy.print(SecretCount);

  arduboy.setCursor(0, 38);
  arduboy.print(bossUnlocked ? F("Boss unlocked") : F("Keep training"));
  arduboy.setCursor(0, 56);
  arduboy.print(F("A/B Back"));
  arduboy.display();
}

void drawFooter() {
  arduboy.setCursor(0, 56);
  arduboy.print(F("High "));
  arduboy.print(saveData.highScore);
}

void drawChargeStatus() {
  if (Moves[selectedMove].needsLeftCharge) {
    arduboy.setCursor(76, 56);
    arduboy.print(leftChargeReady ? F("CHG OK") : F("CHG <"));
  } else if (Moves[selectedMove].needsDownCharge) {
    arduboy.setCursor(76, 56);
    arduboy.print(downChargeReady ? F("CHG OK") : F("CHG D"));
  }
}

void drawMenuItem(uint8_t index, uint8_t y, const __FlashStringHelper *label) {
  arduboy.setCursor(8, y);
  arduboy.print(index == menuIndex ? F(">") : F(" "));
  arduboy.print(label);
}

void drawMenuItem(uint8_t index, uint8_t y, const char *label) {
  arduboy.setCursor(8, y);
  arduboy.print(index == menuIndex ? F(">") : F(" "));
  arduboy.print(label);
}

void drawMoveSequence(const Move &move, uint8_t x, uint8_t y) {
  if (move.needsLeftCharge) {
    drawChargeIcon(x, y, InputLeft);
    x += 14;
  }
  if (move.needsDownCharge) {
    drawChargeIcon(x, y, InputDown);
    x += 14;
  }

  for (uint8_t i = 0; i < move.length; i++) {
    drawInputIcon(move.sequence[i], x + i * 13, y);
  }
}

void drawHistory(uint8_t x, uint8_t y) {
  uint8_t shown = inputCount < 10 ? inputCount : 10;
  uint8_t start = inputCount - shown;

  for (uint8_t i = 0; i < shown; i++) {
    drawInputIcon(inputHistory[start + i], x + i * 12, y);
  }
}

void drawChargeIcon(uint8_t x, uint8_t y, InputCode direction) {
  arduboy.drawRect(x, y, 10, 10, WHITE);
  drawInputIcon(direction, x + 1, y + 1);
}

void drawInputIcon(InputCode input, uint8_t x, uint8_t y) {
  switch (input) {
    case InputUp:
      arduboy.drawLine(x + 4, y, x + 4, y + 8, WHITE);
      arduboy.drawLine(x + 4, y, x + 1, y + 3, WHITE);
      arduboy.drawLine(x + 4, y, x + 7, y + 3, WHITE);
      break;
    case InputDown:
      arduboy.drawLine(x + 4, y, x + 4, y + 8, WHITE);
      arduboy.drawLine(x + 4, y + 8, x + 1, y + 5, WHITE);
      arduboy.drawLine(x + 4, y + 8, x + 7, y + 5, WHITE);
      break;
    case InputLeft:
      arduboy.drawLine(x, y + 4, x + 8, y + 4, WHITE);
      arduboy.drawLine(x, y + 4, x + 3, y + 1, WHITE);
      arduboy.drawLine(x, y + 4, x + 3, y + 7, WHITE);
      break;
    case InputRight:
      arduboy.drawLine(x, y + 4, x + 8, y + 4, WHITE);
      arduboy.drawLine(x + 8, y + 4, x + 5, y + 1, WHITE);
      arduboy.drawLine(x + 8, y + 4, x + 5, y + 7, WHITE);
      break;
    case InputDownRight:
      arduboy.drawLine(x + 1, y + 1, x + 8, y + 8, WHITE);
      arduboy.drawLine(x + 8, y + 8, x + 4, y + 8, WHITE);
      arduboy.drawLine(x + 8, y + 8, x + 8, y + 4, WHITE);
      break;
    case InputDownLeft:
      arduboy.drawLine(x + 8, y + 1, x + 1, y + 8, WHITE);
      arduboy.drawLine(x + 1, y + 8, x + 5, y + 8, WHITE);
      arduboy.drawLine(x + 1, y + 8, x + 1, y + 4, WHITE);
      break;
    case InputA:
      arduboy.setCursor(x, y);
      arduboy.print(F("A"));
      break;
    case InputB:
      arduboy.setCursor(x, y);
      arduboy.print(F("B"));
      break;
  }
}

uint8_t centeredX(const char *text) {
  uint8_t width = strlen(text) * 6;
  if (width >= ScreenWidth) {
    return 0;
  }
  return (ScreenWidth - width) / 2;
}

uint8_t countSecrets() {
  uint8_t count = 0;
  for (uint8_t i = 0; i < SecretCount; i++) {
    if ((saveData.secretsFound & (1 << i)) != 0) {
      count++;
    }
  }
  return count;
}
