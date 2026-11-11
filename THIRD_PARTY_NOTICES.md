# Third-party notices

LevelSync depends on third-party components that retain their own copyrights and licenses.

## JUCE 9.0.5

Copyright © Raw Material Software Limited.

JUCE is dual-licensed under the GNU Affero General Public License v3.0 and a commercial JUCE license. This repository's open-source build uses JUCE under AGPLv3.

- License included with JUCE: `JUCE/LICENSE.md`
- Complete dependency inventory: `JUCE/JUCE.spdx.json`
- Website: <https://juce.com/>
- JUCE 9 license: <https://juce.com/legal/juce-9-licence/>

JUCE source code is not maintained in this repository. Use JUCE 9.0.5 and configure `JUCE_DIR` in CMake to reproduce the build. When distributing binaries under the AGPL, provide equivalent access to the corresponding JUCE source used for that build.

## Steinberg VST 3 SDK

JUCE 9.0.5 includes portions of the VST 3 SDK used to build the VST3 format.

MIT License

Copyright © 2025 Steinberg Media Technologies GmbH

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

## Windows system libraries

The build uses libraries supplied by the Windows SDK and Microsoft Visual C++ runtime. Those components are distributed under Microsoft's terms and are not part of the LevelSync source code.

## Inno Setup

The Windows VST3 installer is compiled with Inno Setup. The compiler is not bundled with this repository; only the generated installer is distributed.

Copyright © 1997-2026 Jordan Russell and Inno Setup contributors.

- Website and downloads: <https://jrsoftware.org/isinfo.php>
- Inno Setup 6 license: <https://jrsoftware.org/files/is6-license.txt>
- Inno Setup 7 license: <https://jrsoftware.org/files/is7-license.txt>

Anyone reusing the workflow must review the terms of the installed Inno Setup version, especially for commercial use.

## Trademarks

JUCE, VST, VST3, and the names of referenced DAWs are trademarks of their respective owners. Their use identifies compatibility only and does not imply endorsement.
