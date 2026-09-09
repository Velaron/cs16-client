#!/bin/bash

set -eu

YY_THUNKS_VERSION=v1.2.1
YY_THUNKS_OBJ_SHA256=51a13c4b667c82181d357e274a50308a273be63f5f85ae958465774857721573

cd "$GITHUB_WORKSPACE"
mkdir -p 3rdparty/yy-thunks

# YY-Thunks obj for Windows XP support
curl -L "https://github.com/Chuyu-Team/YY-Thunks/releases/download/${YY_THUNKS_VERSION}/YY-Thunks-Objs.zip" -o yy-thunks.zip
unzip -q -j -o yy-thunks.zip 'objs/x86/YY_Thunks_for_WinXP.obj' -d 3rdparty/yy-thunks
rm yy-thunks.zip

echo "YY_THUNKS_VERSION=${YY_THUNKS_VERSION}" >> "$GITHUB_ENV"
echo "YY_THUNKS_OBJ_SHA256=${YY_THUNKS_OBJ_SHA256}" >> "$GITHUB_ENV"