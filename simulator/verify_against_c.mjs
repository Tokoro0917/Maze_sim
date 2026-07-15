// Compiles and runs the real Maze_Dijkstra.c (with -DSIM_EXPORT, which makes
// it additionally dump its internal state as JSON) and diffs that output
// against the JavaScript port embedded in simulator/index.html. This is the
// authoritative check that the browser simulator matches the actual program,
// not just a hand-reviewed approximation of it.
//
// Usage:  node simulator/verify_against_c.mjs
// Requires a C compiler named "gcc" on PATH (e.g. WinLibs/MinGW-w64, or any
// gcc/clang on Linux/macOS).

import { execFileSync } from "node:child_process";
import { readFileSync, mkdtempSync, rmSync } from "node:fs";
import { tmpdir } from "node:os";
import { join, dirname } from "node:path";
import { fileURLToPath } from "node:url";
import vm from "node:vm";

const here = dirname(fileURLToPath(import.meta.url));
const repoRoot = join(here, "..");
const cSource = join(repoRoot, "Maze_Dijkstra.c");
const htmlPath = join(here, "index.html");

function fail(msg) {
  console.error(`❌ ${msg}`);
  process.exit(1);
}

// --- 1) Compile & run the real C program ---
const workDir = mkdtempSync(join(tmpdir(), "maze-verify-"));
const exePath = join(workDir, process.platform === "win32" ? "maze_sim_export.exe" : "maze_sim_export");

try {
  execFileSync("gcc", ["-DSIM_EXPORT", cSource, "-o", exePath], { stdio: "pipe" });
} catch (e) {
  fail(
    "gcc でのコンパイルに失敗しました。C コンパイラ (gcc) が PATH に必要です。\n" +
      (e.stderr ? e.stderr.toString() : e.message)
  );
}

try {
  execFileSync(exePath, [], { cwd: workDir, stdio: "pipe" });
} catch (e) {
  fail("コンパイルした実行ファイルの実行に失敗しました。\n" + (e.stderr ? e.stderr.toString() : e.message));
}

let cResult;
try {
  cResult = JSON.parse(readFileSync(join(workDir, "sim_export.json"), "utf8"));
} catch (e) {
  fail("sim_export.json の読み込み/パースに失敗しました。Maze_Dijkstra.c の SIM_EXPORT ブロックを確認してください。\n" + e.message);
}
rmSync(workDir, { recursive: true, force: true });

// --- 2) Extract & run the pure algorithm from the shipped simulator HTML ---
const html = readFileSync(htmlPath, "utf8");
const scriptMatch = html.match(/<script>([\s\S]*)<\/script>/);
if (!scriptMatch) fail("simulator/index.html から <script> ブロックを抽出できませんでした。");
const fullScript = scriptMatch[1];
const marker = "/* ============================================================\n   2)";
const cut = fullScript.indexOf(marker);
if (cut === -1) fail("simulator/index.html 内のアルゴリズム部分の境界マーカーが見つかりませんでした（HTML構造が変わった可能性があります）。");
const algoSource = fullScript.slice(0, cut);

const sandbox = {};
vm.createContext(sandbox);
try {
  vm.runInContext(algoSource + "\nthis.makeMaze = makeMaze; this.solveMaze = solveMaze;", sandbox);
} catch (e) {
  fail("simulator/index.html のアルゴリズム部分の実行に失敗しました。\n" + e.message);
}
const { makeMaze, solveMaze } = sandbox;
const jsResult = solveMaze(makeMaze());

// --- 3) Compare ---
function flattenCost(grid) {
  const out = [];
  for (let x = 0; x < 17; x++) for (let y = 0; y < 17; y++) out.push(grid[x][y].cost);
  return out;
}

const checks = [
  ["nodeRowCost", flattenCost(jsResult.nodeRow), cResult.nodeRowCost],
  ["nodeColumnCost", flattenCost(jsResult.nodeColumn), cResult.nodeColumnCost],
  ["shortPass", jsResult.shortPass, cResult.shortPass],
  ["cp", jsResult.cp, cResult.cp],
  ["naname", jsResult.naname, cResult.naname],
  ["finalX", [jsResult.finalX], [cResult.finalX]],
  ["finalY", [jsResult.finalY], [cResult.finalY]],
  ["finalDir", [jsResult.finalDir], [cResult.finalDir]],
];

let allOk = true;
for (const [name, jsArr, cArr] of checks) {
  if (jsArr.length !== cArr.length) {
    allOk = false;
    console.error(`❌ ${name}: 長さが不一致 (JS=${jsArr.length}, C=${cArr.length})`);
    continue;
  }
  for (let i = 0; i < jsArr.length; i++) {
    if (jsArr[i] !== cArr[i]) {
      allOk = false;
      console.error(`❌ ${name}[${i}] が不一致: JS=${jsArr[i]}, C(実バイナリ)=${cArr[i]}`);
      break;
    }
  }
}

if (allOk) {
  console.log("✅ simulator/index.html の JS 移植は、実際にコンパイル・実行した Maze_Dijkstra.c の出力と完全に一致しました。");
  console.log(`   (cost grid, PASS/CP/NANAME 配列, 最終位置・方向を含む ${checks.length} 項目を照合)`);
  process.exit(0);
} else {
  fail("上記の不一致が見つかりました。JS 移植を Maze_Dijkstra.c に合わせて修正してください。");
}
