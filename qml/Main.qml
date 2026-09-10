import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ApplicationWindow {
    id: window
    width: 1360
    height: 820
    visible: true
    title: "TinyKernel — TK-0001 causal space"
    color: "#101720"

    component Panel: Rectangle {
        color: "#18232f"
        radius: 8
        border.color: "#314255"
        border.width: 1
    }

    header: ToolBar {
        background: Rectangle { color: "#16202a" }
        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 18
            anchors.rightMargin: 18
            Label { text: "TinyKernel"; color: "#f5f7fa"; font.pixelSize: 20; font.bold: true }
            Label { text: "TK-0001  •  TK-O v0.2.0"; color: "#7fd1c8"; Layout.fillWidth: true }
            Label { text: "READY ≠ COMPLETE"; color: "#e4b85f"; font.bold: true }
        }
    }

    GridLayout {
        anchors.fill: parent
        anchors.margins: 14
        columns: 3
        rows: 2
        columnSpacing: 12
        rowSpacing: 12

        Panel {
            Layout.preferredWidth: 330
            Layout.fillHeight: true
            ColumnLayout {
                anchors.fill: parent; anchors.margins: 14; spacing: 9
                Label { text: "FENÔMENO / CONTEXTO / PERFIL"; color: "#7fd1c8"; font.bold: true }
                Label { text: bridge.phenomenon; color: "#f4f7fa"; wrapMode: Text.WordWrap; Layout.fillWidth: true }
                Rectangle { color: "#314255"; height: 1; Layout.fillWidth: true }
                Label { text: bridge.context; color: "#bdc8d5"; wrapMode: Text.WordWrap; Layout.fillWidth: true }
                ScrollView { Layout.fillWidth: true; Layout.fillHeight: true
                    Label { text: bridge.profile; color: "#bdc8d5"; wrapMode: Text.WordWrap; width: 285 }
                }
            }
        }

        Panel {
            Layout.fillWidth: true
            Layout.fillHeight: true
            ColumnLayout {
                anchors.fill: parent; anchors.margins: 14
                Label { text: "CAUSAL SPACE  Gₚ = (R, I)"; color: "#7fd1c8"; font.bold: true }
                Item {
                    Layout.fillWidth: true; Layout.fillHeight: true
                    Repeater {
                        model: bridge.interventions
                        delegate: Button {
                            required property var modelData
                            x: 160; y: modelData.status === "performed" ? (modelData.kind === "replace" ? 72 : 222) : 340 + index * 34
                            width: 140; height: 28
                            text: modelData.kind + " · " + modelData.status
                            font.pixelSize: 10
                            onClicked: bridge.selectEntity(modelData.id)
                        }
                    }
                    Repeater {
                        model: bridge.realizations
                        delegate: Rectangle {
                            required property var modelData
                            x: modelData.x; y: modelData.y; width: 190; height: 76; radius: 9
                            color: modelData.outcome === "preserving" ? "#204d40" : modelData.outcome === "ruptured" ? "#623737" : "#3b4652"
                            border.color: modelData.outcome === "preserving" ? "#65c394" : modelData.outcome === "ruptured" ? "#df7770" : "#8795a4"
                            Column { anchors.centerIn: parent; width: parent.width - 16; spacing: 4
                                Label { text: modelData.label; color: "white"; width: parent.width; elide: Text.ElideRight; horizontalAlignment: Text.AlignHCenter }
                                Label { text: modelData.outcome; color: "#d4dde7"; width: parent.width; horizontalAlignment: Text.AlignHCenter; font.pixelSize: 11 }
                            }
                            MouseArea { anchors.fill: parent; onClicked: bridge.selectEntity(modelData.id) }
                        }
                    }
                }
            }
        }

        Panel {
            Layout.preferredWidth: 350
            Layout.fillHeight: true
            ColumnLayout {
                anchors.fill: parent; anchors.margins: 14
                Label { text: "RUN / EVIDÊNCIA"; color: "#7fd1c8"; font.bold: true }
                ListView {
                    Layout.fillWidth: true; Layout.preferredHeight: 190; clip: true; model: bridge.runs; spacing: 5
                    delegate: Rectangle { required property var modelData; width: ListView.view.width; height: 54; radius: 5; color: "#202e3c"
                        Column { anchors.fill: parent; anchors.margins: 7
                            Label { text: modelData.id; color: "#f4f7fa"; font.pixelSize: 11 }
                            Label { text: modelData.evidenceCount + " registros de evidência imutável"; color: "#9fb0c1"; font.pixelSize: 11 }
                        }
                    }
                }
                Label { text: "PROVENIÊNCIA SELECIONADA"; color: "#7fd1c8"; font.bold: true }
                ScrollView { Layout.fillWidth: true; Layout.fillHeight: true
                    Label { text: bridge.selectedDetails; color: "#bdc8d5"; wrapMode: Text.WordWrap; width: 310 }
                }
            }
        }

        Panel {
            Layout.columnSpan: 2
            Layout.fillWidth: true
            Layout.preferredHeight: 230
            ColumnLayout { anchors.fill: parent; anchors.margins: 14
                Label { text: "CLAIMS"; color: "#7fd1c8"; font.bold: true }
                ListView { Layout.fillWidth: true; Layout.fillHeight: true; model: bridge.claims; spacing: 5; clip: true
                    delegate: Rectangle { required property var modelData; width: ListView.view.width; height: 48; radius: 5; color: "#202e3c"
                        RowLayout { anchors.fill: parent; anchors.margins: 8
                            Label { text: modelData.level; color: "#e4b85f"; font.bold: true }
                            Label { text: modelData.assertion; color: "#f4f7fa"; Layout.fillWidth: true; elide: Text.ElideRight }
                            Label { text: modelData.status; color: modelData.status === "supported" ? "#65c394" : "#e4b85f" }
                        }
                    }
                }
            }
        }

        Panel {
            Layout.fillWidth: true
            Layout.preferredHeight: 230
            ColumnLayout { anchors.fill: parent; anchors.margins: 14
                Label { text: "FRONTIER"; color: "#7fd1c8"; font.bold: true }
                Label { text: bridge.frontier; color: "#f4f7fa"; wrapMode: Text.WordWrap; Layout.fillWidth: true }
                Item { Layout.fillHeight: true }
                Label { text: "INCOMPLETE BY DESIGN"; color: "#e4b85f"; font.bold: true }
            }
        }
    }
}
