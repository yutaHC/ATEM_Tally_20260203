# Bitfocus Companion 設定ガイド

このドキュメントでは、Bitfocus Companionから `ATEM_Tally_20260203` を制御するための設定方法（ロジック）を説明します。

## 1. モジュールの追加
Companionの「Connections」タブから、標準で用意されている汎用通信モジュールを追加します。

- **Category:** Generic
- **Module:** `Generic: TCP and UDP commands`

追加後、このモジュールの設定画面（Configuration）で以下を入力して「Save」します。
- **Target IP:** マイコン（M5Atom）に割り当てられたIPアドレス
- **Target Port:** `8888`

## 2. アクションの作成（ボタン手動操作）
ボタンを押して手動でタリーを切り替える場合、「Buttons」タブでボタンを作成し、Actionsに以下を設定します。

### Program (赤点灯) ボタン
- **Action:** `Generic: TCP and UDP commands: Send UDP command`
- **Command:** `pgm`

### Preview (緑点灯) ボタン
- **Action:** `Generic: TCP and UDP commands: Send UDP command`
- **Command:** `pvw`

### OFF (消灯) ボタン
- **Action:** `Generic: TCP and UDP commands: Send UDP command`
- **Command:** `off`

### Identify (本体の場所確認 / 青色点滅)
- **Action:** `Generic: TCP and UDP commands: Send UDP command`
- **Command:** `identify`
※ 機材セットアップ時に「どのIPがどのカメラのタリーか」探すときに便利です。

### 輝度の変更 (例：半分の明るさ)
- **Action:** `Generic: TCP and UDP commands: Send UDP command`
- **Command:** `dim:128`
※ `0` (消灯) 〜 `255` (最大) の間で指定できます。

---

## 3. ATEMスイッチャーと連動させる（自動化ロジック）
Companionの **「Triggers」** 機能を使うと、ATEMでカメラを切り替えた瞬間に、自動でM5AtomへUDPコマンドを送信できます。

### Triggersの作成例：「カメラ1がProgramに選ばれたらタリーを赤くする」

1. Companionの **Triggers** タブを開き、新しくトリガーを作成します。
2. **Events (When):**
   - `ATEM: In Program` を選択
   - `Input`: Camera 1 等を選択
3. **Actions (Then):**
   - `Generic: TCP and UDP commands: Send UDP command` を選択
   - **Command:** `pgm`

### 複数カメラのタリーを連動させる場合
- 各カメラ（M5Atom）ごとにIPアドレスが異なるため、**カメラの台数分「Generic: TCP and UDP commands」のConnectionを作成**します（例: `Tally-Cam1`, `Tally-Cam2`）。
- そして、Triggersで「Camera 1がPGMになったら `Tally-Cam1` に `pgm` を送る」「Camera 2がPGMになったら `Tally-Cam2`に `pgm`、`Tally-Cam1`に `off`（または `pvw`）を送る」といったロジックを組みます。

> **Tips:** 
> ATEMプロトコルではなくCompanionを経由するため、OBS StudioやvMix、Rolandスイッチャーのタリーとしても全く同じロジックで流用可能です！
