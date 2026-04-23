# AI to Arduino Flow

## Short Idea

The AI sends a normal text string to the ESP32 using UDP.

The ESP32 is the first place where the string is split into parts.

After that, the ESP32 converts those parts into an array-shaped string like:

```text
["FACE","0"]
["OBJ","R","11"]
```

Then the Arduino receives that text line, parses it again into a local `String tokens[]`, and runs the correct action.

## Important Note

Between the ESP32 and Arduino, the message is still text.

It is not a real C++ array traveling through the wire.

It is a serialized string that looks like an array.

Example:

```text
["OBJ","R","11"]
```

When Arduino receives this line, it creates its own local array from that text.

## File Roles

### `mohsen_esp32_v5.ino`

- Receives the message from AI
- Splits the original AI string
- Decides which mode it is
- For object detection, calculates direction and angle
- Converts the result to an array-shaped string
- Sends that string to Arduino

### `mohsen_arduino_v5.ino`

- Receives the string from ESP32 over serial
- Waits until the full line arrives
- Parses the array-shaped string into local tokens
- Chooses the correct action
- Runs face or object behavior

## Full Flow

### Step 1: AI sends a string

Examples:

```text
FACE,0
FACE,1
O,220,80,R
```

Meaning:

- `FACE,0` = face mode, friend
- `FACE,1` = face mode, stranger
- `O,220,80,R` = object mode, object data from AI

## ESP32 Flow

### 1. `loop()`

Function: `loop()` in `mohsen_esp32_v5.ino`

What it does:

- Waits for a UDP packet from AI
- Reads the text into `buf`
- Converts it to `raw`
- Removes spaces/newlines using `trim()`

Flow:

```text
AI sends string
-> loop()
-> udp.read(...)
-> raw = String(buf)
-> raw.trim()
```

### 2. `splitToArray(raw, ',', tokens, MAX_TOKENS)`

Function: `splitToArray()`

What it does:

- Splits the AI string by comma `,`
- Stores each part inside `tokens[]`

Example:

```text
raw = "FACE,0"
tokens[0] = "FACE"
tokens[1] = "0"
```

```text
raw = "O,220,80,R"
tokens[0] = "O"
tokens[1] = "220"
tokens[2] = "80"
tokens[3] = "R"
```

### 3. Mode check inside `loop()`

Function: `loop()`

What it does:

- Reads `tokens[0]`
- Decides where to send the flow

Flow:

```text
if tokens[0] == "O"
-> handleObject(tokens, count)

if tokens[0] == "FACE"
-> handleFace(tokens, count)
```

## Face Flow

### 4. `handleFace(tokens, count)`

Function: `handleFace()` in ESP32 file

What it does:

- Reads `tokens[1]`
- Saves it as `cat`
- Builds a local string array:

```text
out[0] = "FACE"
out[1] = cat
```

If AI sent:

```text
FACE,0
```

Then ESP makes:

```text
["FACE","0"]
```

### 5. `buildArrayString(items, count)`

Function: `buildArrayString()`

What it does:

- Takes the local array like `{"FACE", "0"}`
- Converts it into one text string:

```text
["FACE","0"]
```

### 6. `sendArrayToArduino(items, count)`

Function: `sendArrayToArduino()`

What it does:

- Calls `buildArrayString()`
- Sends the final text to Arduino using:

```cpp
ArduinoSerial.println(cmd);
```

So the serial message becomes:

```text
["FACE","0"]
```

## Object Detection Flow

### 4. `handleObject(tokens, count)`

Function: `handleObject()` in ESP32 file

What it does:

- Reads the object position from `tokens[]`
- Uses `tokens[1]` as `objX`
- Uses `tokens[2]` as `objY`
- Calculates:
  - `dX`
  - `angleDeg`
  - `side`

Important:

The ESP32 creates the final object command for Arduino.

So Arduino does not receive the original AI string like:

```text
O,220,80,R
```

Arduino receives the final translated message:

```text
["OBJ","R","11"]
```

### 5. Inside `handleObject()`

ESP creates:

```text
out[0] = "OBJ"
out[1] = side
out[2] = angleDeg as text
```

Example:

```text
AI input: O,220,80,R
ESP result: ["OBJ","R","11"]
```

### 6. `buildArrayString()` and `sendArrayToArduino()`

Same idea as face mode:

```text
local array -> array-shaped text -> serial send to Arduino
```

## Arduino Flow

### 1. `loop()`

Function: `loop()` in `mohsen_arduino_v5.ino`

What it does:

- Reads serial one character at a time
- Adds each character to `serialBuf`
- Waits until newline `\n`
- Then sends the full line to `processCmd(serialBuf)`

Flow:

```text
ESP sends: ["FACE","0"]\n
-> Arduino loop()
-> Serial.read()
-> serialBuf += c
-> when c == '\n'
-> processCmd(serialBuf)
```

### 2. `processCmd(cmd)`

Function: `processCmd()`

What it does:

- Receives the full text line
- Makes a local array:

```cpp
String tokens[MAX_TOKENS];
```

- Calls:

```cpp
parseCmdTokens(cmd, tokens, MAX_TOKENS);
```

### 3. `parseCmdTokens(input, out, maxCount)`

Function: `parseCmdTokens()`

What it does:

- Checks if the received command starts with `[` 
- If yes, it uses `parseQuotedArray()`
- If not, it can still use `splitLegacyCmd()` for old comma format

Flow:

```text
if input starts with "["
-> parseQuotedArray(...)

else
-> splitLegacyCmd(...)
```

## Arduino Parsing the Array

### 4. `parseQuotedArray(input, out, maxCount)`

Function: `parseQuotedArray()`

What it does:

- Reads the string character by character
- Finds text between double quotes `"`
- Stores each quoted value in `out[]`

Example:

```text
input = ["OBJ","R","11"]
```

After parsing:

```text
tokens[0] = "OBJ"
tokens[1] = "R"
tokens[2] = "11"
```

For face:

```text
input = ["FACE","0"]

tokens[0] = "FACE"
tokens[1] = "0"
```

### 5. Back to `processCmd()`

After parsing, Arduino checks `tokens[0]`.

Flow:

```text
if mode == "OBJ"
-> handleObj(side, angle)

else if mode == "FACE"
-> handleFace(cat)

else if mode == "SYS"
-> handle system message
```

## Final Arduino Actions

### `handleObj(side, angle)`

Function: `handleObj()`

What it does:

- Uses `side`
- Uses `angle`
- Updates sound and screen
- Later the rest of the team can connect this to motors

Example:

```text
["OBJ","R","11"]
-> side = "R"
-> angle = 11
-> handleObj("R", 11)
```

### `handleFace(cat)`

Function: `handleFace()`

What it does:

- Converts `"0"` or `"1"` to integer
- If `0` -> friend
- If `1` -> stranger
- Updates sound and screen

Example:

```text
["FACE","0"]
-> cat = 0
-> handleFace(0)
```

## Flow Summary

### Face Mode

```text
AI sends "FACE,0"
-> ESP loop() receives it
-> splitToArray()
-> handleFace()
-> buildArrayString()
-> sendArrayToArduino()
-> Arduino loop() collects full serial line
-> processCmd()
-> parseCmdTokens()
-> parseQuotedArray()
-> handleFace(0)
```

### Object Mode

```text
AI sends "O,220,80,R"
-> ESP loop() receives it
-> splitToArray()
-> handleObject()
-> calculate side and angle
-> buildArrayString()
-> sendArrayToArduino()
-> Arduino loop() collects full serial line
-> processCmd()
-> parseCmdTokens()
-> parseQuotedArray()
-> handleObj("R", 11)
```

## Meaning of the Main Values

| Value | Meaning |
|---|---|
| `FACE` | Face recognition mode |
| `OBJ` | Final object command for Arduino |
| `SYS` | System message |
| `0` | Friend |
| `1` | Stranger |
| `R` | Right |
| `L` | Left |
| `C` | Centered |
| `11` | Angle in degrees |

## Very Short Final Idea

```text
AI sends plain comma string
-> ESP splits it
-> ESP converts it to array-shaped string
-> Arduino parses that string into local tokens[]
-> Arduino runs the correct action
```
