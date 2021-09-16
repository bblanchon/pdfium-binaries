#!/bin/bash

./build.sh

[ -n "$TARGET_CPU" ] || ./test.sh
