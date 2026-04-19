param(
    [string]$repoName = "SmartScope",
    [string]$username = "",
    [string]$token = "",
    [switch]$ForceOverwrite,
    [switch]$Push,
    [switch]$CreateRepo,
    [switch]$DryRun
)

$ErrorActionPreference = "Stop"

function Write-Info($msg) {
    Write-Host "[init_full] $msg"
}

function Write-FileSmart {
    param(
        [Parameter(Mandatory = $true)][string]$Path,
        [Parameter(Mandatory = $true)][string]$Content,
        [switch]$Force
    )

    if ((Test-Path $Path) -and -not $Force) {
        Write-Info "Skip existing: $Path"
        return
    }

    if ($DryRun) {
        Write-Info "DryRun write: $Path"
        return
    }

    $dir = Split-Path -Parent $Path
    if ($dir -and -not (Test-Path $dir)) {
        New-Item -ItemType Directory -Force -Path $dir | Out-Null
    }

    Set-Content -Path $Path -Value $Content -Encoding utf8
    Write-Info "Write: $Path"
}

# Use script directory as repository root to avoid creating/deleting sibling folders.
$repoRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
Set-Location $repoRoot

if (-not $token) {
    $token = $env:GITHUB_TOKEN
}

Write-Info "Repo root: $repoRoot"

# Basic scaffold directories (safe in current repo)
if (-not $DryRun) {
    New-Item -ItemType Directory -Force -Path "src", ".github", ".github/workflows" | Out-Null
}

$cmake = @"
cmake_minimum_required(VERSION 3.16)
project(SmartScope)

set(CMAKE_CXX_STANDARD 17)
find_package(Qt6 REQUIRED COMPONENTS Widgets)

add_executable(SmartScope
src/main.cpp
src/MainWindow.cpp
src/OscilloscopeWidget.cpp
src/DataEngine.cpp
src/TriggerEngine.cpp
)

target_link_libraries(SmartScope Qt6::Widgets)
"@

$mainCpp = @"
#include <QApplication>
#include "MainWindow.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    MainWindow w;
    w.show();
    return app.exec();
}
"@

$dataEngineH = @"
#pragma once
#include <mutex>
#include <vector>

struct Sample {
    double t;
    int v;
};

class DataEngine {
public:
    void push(int v);
    std::vector<Sample> get();

private:
    std::vector<Sample> buf;
    std::mutex mtx;
    double t = 0;
};
"@

$dataEngineCpp = @"
#include "DataEngine.h"

void DataEngine::push(int v) {
    std::lock_guard<std::mutex> l(mtx);
    buf.push_back({t, v});
    t += 0.01;

    if (buf.size() > 2000) {
        buf.erase(buf.begin());
    }
}

std::vector<Sample> DataEngine::get() {
    std::lock_guard<std::mutex> l(mtx);
    return buf;
}
"@

$triggerEngineH = @"
#pragma once
#include <deque>
#include <vector>

class TriggerEngine {
public:
    int level = 2500;
    bool rising = true;
    int pre = 200;
    int post = 800;

    void push(int v);
    bool ready();
    void reset();

private:
    std::deque<int> preBuf;
    std::vector<int> cap;
    bool triggered = false;
    bool done = false;
    int last = 0;
};
"@

$triggerEngineCpp = @"
#include "TriggerEngine.h"

void TriggerEngine::reset() {
    preBuf.clear();
    cap.clear();
    triggered = false;
    done = false;
}

void TriggerEngine::push(int v) {
    if (done) return;

    preBuf.push_back(v);
    if (preBuf.size() > pre) preBuf.pop_front();

    if (!triggered) {
        bool hit = rising ?
            (last < level && v >= level) :
            (last > level && v <= level);

        if (hit) {
            triggered = true;
            cap.insert(cap.end(), preBuf.begin(), preBuf.end());
        }
    }

    if (triggered) {
        cap.push_back(v);
        if (cap.size() >= pre + post) done = true;
    }

    last = v;
}

bool TriggerEngine::ready() {
    return done;
}
"@

$oscWidgetH = @"
#pragma once
#include <QWidget>
#include "DataEngine.h"
#include "TriggerEngine.h"

class OscilloscopeWidget : public QWidget {
public:
    OscilloscopeWidget(DataEngine* e, TriggerEngine* t);

protected:
    void paintEvent(QPaintEvent*) override;

private:
    DataEngine* engine;
    TriggerEngine* trigger;
};
"@

$oscWidgetCpp = @"
#include "OscilloscopeWidget.h"
#include <QPainter>

OscilloscopeWidget::OscilloscopeWidget(DataEngine* e, TriggerEngine* t)
    : engine(e), trigger(t) {}

void OscilloscopeWidget::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.fillRect(rect(), Qt::black);

    int w = width(), h = height();
    if (w <= 0 || h <= 0) return;

    p.setPen(QColor(50, 50, 50));
    for (int i = 0; i <= 10; i++) p.drawLine(i * w / 10, 0, i * w / 10, h);
    for (int i = 0; i <= 8; i++) p.drawLine(0, i * h / 8, w, i * h / 8);

    auto data = engine->get();
    if (data.size() < 2) return;

    p.setPen(QPen(QColor(0, 255, 0), 2));
    for (size_t i = 1; i < data.size(); i++) {
        int x1 = (i - 1) * w / static_cast<int>(data.size());
        int x2 = i * w / static_cast<int>(data.size());
        int y1 = h - data[i - 1].v * h / 5000;
        int y2 = h - data[i].v * h / 5000;
        p.drawLine(x1, y1, x2, y2);
    }

    p.setPen(Qt::red);
    p.drawLine(w / 2, 0, w / 2, h);
}
"@

$mainWindowH = @"
#pragma once
#include <QMainWindow>
#include "DataEngine.h"
#include "TriggerEngine.h"

class MainWindow : public QMainWindow {
public:
    MainWindow();

private:
    DataEngine engine;
    TriggerEngine trigger;
};
"@

$mainWindowCpp = @"
#include "MainWindow.h"
#include "OscilloscopeWidget.h"
#include <QTimer>

MainWindow::MainWindow() {
    auto* osc = new OscilloscopeWidget(&engine, &trigger);
    setCentralWidget(osc);
    resize(1000, 600);

    auto* t = new QTimer(this);
    connect(t, &QTimer::timeout, [&, osc]() {
        static int v = 0;
        v = (v + 300) % 5000;

        engine.push(v);
        trigger.push(v);

        if (trigger.ready()) {
            trigger.reset();
        }

        osc->update();
    });

    t->start(20);
}
"@

$workflowYml = @"
name: Build

on:
  push:
  pull_request:

jobs:
  build:
    runs-on: windows-latest

    steps:
      - uses: actions/checkout@v4

      - uses: jurplel/install-qt-action@v4
        with:
          version: '6.6.0'

      - run: cmake -B build -S .
      - run: cmake --build build --config Release
"@

Write-FileSmart -Path "CMakeLists.txt" -Content $cmake -Force:$ForceOverwrite
Write-FileSmart -Path "src/main.cpp" -Content $mainCpp -Force:$ForceOverwrite
Write-FileSmart -Path "src/DataEngine.h" -Content $dataEngineH -Force:$ForceOverwrite
Write-FileSmart -Path "src/DataEngine.cpp" -Content $dataEngineCpp -Force:$ForceOverwrite
Write-FileSmart -Path "src/TriggerEngine.h" -Content $triggerEngineH -Force:$ForceOverwrite
Write-FileSmart -Path "src/TriggerEngine.cpp" -Content $triggerEngineCpp -Force:$ForceOverwrite
Write-FileSmart -Path "src/OscilloscopeWidget.h" -Content $oscWidgetH -Force:$ForceOverwrite
Write-FileSmart -Path "src/OscilloscopeWidget.cpp" -Content $oscWidgetCpp -Force:$ForceOverwrite
Write-FileSmart -Path "src/MainWindow.h" -Content $mainWindowH -Force:$ForceOverwrite
Write-FileSmart -Path "src/MainWindow.cpp" -Content $mainWindowCpp -Force:$ForceOverwrite
Write-FileSmart -Path ".github/workflows/build.yml" -Content $workflowYml -Force:$ForceOverwrite

# Git init/add/commit in current repo.
if (-not (Test-Path ".git")) {
    if ($DryRun) {
        Write-Info "DryRun git init"
    } else {
        git init
    }
}

if ($DryRun) {
    Write-Info "DryRun git add ."
} else {
    git add .
}

if (-not $DryRun) {
    git diff --cached --quiet
    if ($LASTEXITCODE -ne 0) {
        git commit -m "init full scaffold"
        Write-Info "Committed scaffold changes"
    } else {
        Write-Info "No staged changes to commit"
    }
}

# Optional GitHub create/push
if ($CreateRepo) {
    if (-not $token) {
        throw "CreateRepo needs -token or GITHUB_TOKEN"
    }

    if (-not $username) {
        throw "CreateRepo needs -username"
    }

    $body = @{ name = $repoName; private = $false } | ConvertTo-Json

    try {
        if (-not $DryRun) {
            Invoke-RestMethod `
                -Uri "https://api.github.com/user/repos" `
                -Headers @{
                    Authorization = "token $token"
                    Accept = "application/vnd.github+json"
                } `
                -Method Post `
                -Body $body | Out-Null
        }
        Write-Info "Repository ready: $username/$repoName"
    } catch {
        if ($_.Exception.Response.StatusCode -eq 422) {
            Write-Info "Repository already exists"
        } else {
            throw $_
        }
    }
}

if ($Push) {
    if (-not $username) {
        throw "Push needs -username"
    }

    $remote = "https://github.com/$($username)/$($repoName).git"

    if (-not $DryRun) {
        if (git remote get-url origin 2>$null) {
            git remote set-url origin $remote
        } else {
            git remote add origin $remote
        }

        git branch -M main
        git push -u origin main
    }

    Write-Info "Pushed: https://github.com/$username/$repoName"
}

Write-Info "Done"
