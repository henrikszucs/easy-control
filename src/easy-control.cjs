"use strict";

const os = require("node:os");

const Control = require("./easy-control-" + os.platform() + "-" + os.arch() + ".node");

module.exports = Control;