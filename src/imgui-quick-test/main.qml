/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

import QtQuick 2.0
import ImGui 1.0

Rectangle {
    id: root
    color: "lightGray"

    Rectangle {
        property int d: 600
        id: square
        width: d
        height: d
        anchors.centerIn: parent
        color: "red"
        NumberAnimation on rotation { from: 0; to: 360; duration: 2000; loops: Animation.Infinite; }
    }

    Text {
        anchors.centerIn: parent
        text: "Qt Quick + ImGui"
    }

    Rectangle {
        id: ctr
        border.width: 4
        border.color: "black"
        color: "transparent"

        x: 50
        y: 50
        width: parent.width - 100
        height: parent.height - 200 // exercise scissoring a bit by having topY != bottomY

        SequentialAnimation {
            id: moveAnim
            NumberAnimation {
                target: ctr
                property: "x"
                from: 50
                to: 0
                duration: 1000
            }
            NumberAnimation {
                target: ctr
                property: "x"
                from: 0
                to: ctr.parent.width - ctr.width
                duration: 1000
            }
            NumberAnimation {
                target: ctr
                property: "y"
                from: 50
                to: 0
                duration: 1000
            }
            NumberAnimation {
                target: ctr
                property: "y"
                from: 0
                to: ctr.parent.height - ctr.height
                duration: 1000
            }
            NumberAnimation {
                target: ctr
                property: "y"
                from: ctr.parent.height - ctr.height
                to: 50
                duration: 1000
            }
            NumberAnimation {
                target: ctr
                property: "x"
                from: ctr.parent.width - ctr.width
                to: 50
                duration: 1000
            }
            loops: Animation.Infinite
            running: true
        }

        ImGui {
            objectName: "imgui"
            anchors.fill: parent
            focus: true // for keybord input
            // note that transforms other than 2D translation are NOT supported

            Rectangle {
                color: "blue"
                anchors.centerIn: parent
                width: 50
                height: 50
                opacity: 1 // test if the blue rect shows up on top when it is part of the opaque pass
//                NumberAnimation on opacity {
//                    from: 1; to: 0; duration: 5000
//                    loops: Animation.Infinite
//                }
            }
        }
    }
}
