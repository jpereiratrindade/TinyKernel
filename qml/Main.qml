import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ApplicationWindow {
    id: window
    width: 1440
    height: 900
    minimumWidth: 1320
    minimumHeight: 760
    visible: true
    title: "TinyKernel — TK-0001 causal space"
    color: "#0e1720"

    component Panel: Rectangle {
        color: "#182531"
        radius: 9
        border.color: "#35495c"
        border.width: 1
    }

    component SectionTitle: Label {
        color: "#72d7cf"
        font.pixelSize: 14
        font.bold: true
    }

    component TkButton: Button {
        id: control
        implicitHeight: 34
        padding: 8
        contentItem: Label {
            text: control.text
            color: "#e8f0f6"
            font: control.font
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
        }
        background: Rectangle {
            radius: 6
            color: control.down ? "#176b67" : control.hovered ? "#28566a" : "#223c4c"
            border.color: control.hovered ? "#72d7cf" : "#426074"
        }
    }

    header: ToolBar {
        height: 58
        background: Rectangle { color: "#15222e"; border.color: "#293b4c" }
        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 18
            anchors.rightMargin: 18
            spacing: 12

            Label {
                text: "TinyKernel"
                color: "#f5f8fb"
                font.pixelSize: 22
                font.bold: true
            }
            Label {
                text: "TK-0001  •  TK-O v0.2.0"
                color: "#72d7cf"
                Layout.fillWidth: true
            }
            TkButton {
                text: "Executar TK-0001"
                onClicked: bridge.runInvestigation()
            }
            TkButton {
                text: "Exportar JSON"
                onClicked: bridge.exportInvestigation()
            }
            Label {
                text: "READY ≠ COMPLETE"
                color: "#f3bf4f"
                font.bold: true
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 14
        spacing: 10

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 36
            radius: 6
            color: "#20303e"
            border.color: "#344b5f"

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 12
                anchors.rightMargin: 12
                Label { text: "STATUS"; color: "#72d7cf"; font.bold: true }
                Label {
                    text: bridge.statusMessage
                    color: "#dce5ed"
                    Layout.fillWidth: true
                    elide: Text.ElideRight
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumHeight: 430
            spacing: 12

            Panel {
                Layout.preferredWidth: 330
                Layout.minimumWidth: 300
                Layout.maximumWidth: 370
                Layout.fillHeight: true

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 14
                    spacing: 9

                    SectionTitle { text: "FENÔMENO / CONTEXTO / PERFIL" }
                    Label {
                        text: bridge.phenomenon
                        color: "#f4f7fa"
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                    }
                    Rectangle { color: "#35495c"; height: 1; Layout.fillWidth: true }
                    Label {
                        text: bridge.context
                        color: "#b9c7d4"
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                    }
                    ScrollView {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true
                        Label {
                            text: bridge.profile
                            color: "#b9c7d4"
                            wrapMode: Text.WordWrap
                            width: 288
                        }
                    }
                }
            }

            Panel {
                Layout.fillWidth: true
                Layout.minimumWidth: 560
                Layout.fillHeight: true

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 14
                    spacing: 7

                    RowLayout {
                        Layout.fillWidth: true
                        SectionTitle { text: "CAUSAL SPACE  Gₚ = (R, I)"; Layout.fillWidth: true }
                        Label { text: "clique em nós e intervenções"; color: "#8fa4b6"; font.pixelSize: 11 }
                    }

                    Item {
                        id: graphArea
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        Layout.minimumHeight: 340

                        Canvas {
                            id: edges
                            anchors.fill: parent
                            z: 0
                            onPaint: {
                                const context = getContext("2d")
                                context.reset()
                                context.lineWidth = 2
                                context.strokeStyle = "#56758d"
                                context.fillStyle = "#56758d"
                                const nodes = bridge.realizations
                                const interventions = bridge.interventions
                                for (let i = 0; i < interventions.length; ++i) {
                                    const edge = interventions[i]
                                    if (edge.status !== "performed")
                                        continue
                                    let source = null
                                    let target = null
                                    for (let j = 0; j < nodes.length; ++j) {
                                        if (nodes[j].id === edge.source) source = nodes[j]
                                        if (nodes[j].id === edge.target) target = nodes[j]
                                    }
                                    if (!source || !target)
                                        continue
                                    const sx = source.x + 190
                                    const sy = source.y + 39
                                    const tx = target.x
                                    const ty = target.y + 39
                                    context.beginPath()
                                    context.moveTo(sx, sy)
                                    context.lineTo(tx - 9, ty)
                                    context.stroke()
                                    context.beginPath()
                                    context.moveTo(tx, ty)
                                    context.lineTo(tx - 12, ty - 6)
                                    context.lineTo(tx - 12, ty + 6)
                                    context.closePath()
                                    context.fill()
                                }
                            }
                            Connections {
                                target: bridge
                                function onDataChanged() { edges.requestPaint() }
                            }
                        }

                        Repeater {
                            model: bridge.interventions
                            delegate: TkButton {
                                required property var modelData
                                visible: modelData.status === "performed"
                                x: modelData.x
                                y: modelData.y
                                z: 2
                                width: 138
                                height: 30
                                text: modelData.kind
                                font.pixelSize: 11
                                ToolTip.visible: hovered
                                ToolTip.text: modelData.id + "\nPredição: " + modelData.prediction
                                onClicked: bridge.selectEntity(modelData.id)
                            }
                        }

                        Repeater {
                            model: bridge.realizations
                            delegate: Rectangle {
                                required property var modelData
                                x: modelData.x
                                y: modelData.y
                                z: 3
                                width: 190
                                height: 78
                                radius: 9
                                color: modelData.outcome === "preserving" ? "#1d5041"
                                      : modelData.outcome === "ruptured" ? "#653938" : "#3b4855"
                                border.color: modelData.outcome === "preserving" ? "#62d09b"
                                            : modelData.outcome === "ruptured" ? "#ed7771" : "#8b9baa"
                                border.width: 1

                                Column {
                                    anchors.centerIn: parent
                                    width: parent.width - 16
                                    spacing: 5
                                    Label {
                                        text: modelData.label
                                        color: "white"
                                        width: parent.width
                                        elide: Text.ElideRight
                                        horizontalAlignment: Text.AlignHCenter
                                    }
                                    Label {
                                        text: modelData.outcome
                                        color: "#d3dee7"
                                        width: parent.width
                                        horizontalAlignment: Text.AlignHCenter
                                        font.pixelSize: 11
                                    }
                                }
                                MouseArea {
                                    anchors.fill: parent
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: bridge.selectEntity(modelData.id)
                                }
                            }
                        }

                        ColumnLayout {
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.bottom: parent.bottom
                            spacing: 5

                            Label {
                                text: "FRONTEIRA DE INTERVENÇÕES"
                                color: "#8fa4b6"
                                font.bold: true
                                font.pixelSize: 11
                            }
                            Flow {
                                Layout.fillWidth: true
                                spacing: 6
                                Repeater {
                                    model: bridge.interventions
                                    delegate: TkButton {
                                        required property var modelData
                                        visible: modelData.status !== "performed"
                                        width: visible ? 172 : 0
                                        height: visible ? 32 : 0
                                        text: modelData.kind + " · " + modelData.target
                                        font.pixelSize: 10
                                        onClicked: bridge.selectEntity(modelData.id)
                                    }
                                }
                            }
                        }
                    }
                }
            }

            Panel {
                Layout.preferredWidth: 360
                Layout.minimumWidth: 330
                Layout.maximumWidth: 410
                Layout.fillHeight: true

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 14
                    spacing: 8

                    SectionTitle { text: "RUNS / EVIDÊNCIA" }
                    ListView {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 190
                        clip: true
                        model: bridge.runs
                        spacing: 6

                        delegate: Rectangle {
                            required property var modelData
                            width: ListView.view.width
                            height: 58
                            radius: 6
                            color: runMouse.containsMouse ? "#2a4051" : "#213342"
                            border.color: "#30495c"

                            Column {
                                anchors.fill: parent
                                anchors.margins: 8
                                Label { text: modelData.id; color: "#f4f7fa"; font.pixelSize: 11 }
                                Label {
                                    text: modelData.evidenceCount + " evidências • " + modelData.result
                                    color: "#99afc0"
                                    width: parent.width
                                    elide: Text.ElideRight
                                    font.pixelSize: 10
                                }
                            }
                            MouseArea {
                                id: runMouse
                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                onClicked: bridge.selectEntity(modelData.id)
                            }
                        }
                    }

                    SectionTitle { text: "INSPEÇÃO / PROVENIÊNCIA" }
                    ScrollView {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true
                        Label {
                            text: bridge.selectedDetails
                            color: "#c4d0db"
                            wrapMode: Text.Wrap
                            width: 320
                            font.pixelSize: 11
                        }
                    }
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: 238
            Layout.minimumHeight: 210
            spacing: 12

            Panel {
                Layout.preferredWidth: 560
                Layout.minimumWidth: 490
                Layout.fillHeight: true

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 14
                    SectionTitle { text: "CLAIMS — FORÇA E ESCOPO" }
                    ListView {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        model: bridge.claims
                        spacing: 6
                        clip: true

                        delegate: Rectangle {
                            required property var modelData
                            width: ListView.view.width
                            height: 48
                            radius: 6
                            color: claimMouse.containsMouse ? "#2a4051" : "#213342"

                            RowLayout {
                                anchors.fill: parent
                                anchors.margins: 9
                                Label { text: modelData.level; color: "#f3bf4f"; font.bold: true }
                                Label {
                                    text: modelData.assertion
                                    color: "#f4f7fa"
                                    Layout.fillWidth: true
                                    elide: Text.ElideRight
                                }
                                Label {
                                    text: modelData.status
                                    color: modelData.status === "supported" ? "#62d09b" : "#f3bf4f"
                                }
                            }
                            MouseArea {
                                id: claimMouse
                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                onClicked: bridge.selectEntity(modelData.id)
                            }
                        }
                    }
                }
            }

            Panel {
                Layout.fillWidth: true
                Layout.fillHeight: true

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 14
                    SectionTitle { text: "FRONTIER" }
                    Label {
                        text: bridge.frontier
                        color: "#f4f7fa"
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                    }
                    Rectangle { color: "#35495c"; height: 1; Layout.fillWidth: true }
                    Label {
                        text: "As candidatas são minimais apenas no espaço conhecido e sob Γ.\n"
                            + "Intervenções abertas impedem promoção a L4–L8."
                        color: "#aebdca"
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                    }
                    Item { Layout.fillHeight: true }
                    Label { text: "INCOMPLETE BY DESIGN"; color: "#f3bf4f"; font.bold: true }
                }
            }
        }
    }
}
