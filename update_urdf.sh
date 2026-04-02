#!/usr/bin/env bash
set -e # immediately exit if any command returns non-zero exit code

onshape-to-robot onshape

cp onshape/robot.urdf www/static/
