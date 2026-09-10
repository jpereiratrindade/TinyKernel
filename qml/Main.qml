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
    title: "TinyKernel — Laboratório Experimental de Minimalidade Causal (SisTer)"
    color: "#0b1320"

    component Panel: Rectangle {
        color: "#111c2a"
        radius: 10
        border.color: "#223547"
        border.width: 1
    }

    component SectionTitle: Label {
        color: "#72d7cf"
        font.pixelSize: 13
        font.bold: true
        font.letterSpacing: 0.6
    }

    component TkButton: Button {
        id: control
        implicitHeight: 34
        padding: 8
        contentItem: Label {
            text: control.text
            color: control.enabled ? "#e8f0f6" : "#64748b"
            font.bold: true
            font.pixelSize: 12
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
        }
        background: Rectangle {
            radius: 6
            color: !control.enabled ? "#1e293b" : control.down ? "#0284c7" : control.hovered ? "#0369a1" : "#1e2e40"
            border.color: !control.enabled ? "#334155" : control.hovered ? "#38bdf8" : "#334b63"
        }
    }

    component PrimaryButton: Button {
        id: controlPrimary
        implicitHeight: 34
        padding: 10
        contentItem: Label {
            text: controlPrimary.text
            color: "#ffffff"
            font.bold: true
            font.pixelSize: 12
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }
        background: Rectangle {
            radius: 6
            gradient: Gradient {
                GradientStop { position: 0.0; color: controlPrimary.down ? "#0369a1" : "#0284c7" }
                GradientStop { position: 1.0; color: controlPrimary.down ? "#0284c7" : "#0369a1" }
            }
            border.color: controlPrimary.hovered ? "#7dd3fc" : "#38bdf8"
        }
    }

    // Header Global
    header: ToolBar {
        height: 60
        background: Rectangle {
            color: "#0f172a"
            border.color: "#1e293b"
        }
        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 20
            anchors.rightMargin: 20
            spacing: 14

            MouseArea {
                Layout.preferredWidth: 160
                Layout.preferredHeight: 36
                cursorShape: Qt.PointingHandCursor
                onClicked: bridge.backToHome()

                RowLayout {
                    anchors.fill: parent
                    spacing: 8
                    Label {
                        text: "TinyKernel"
                        color: "#f8fafc"
                        font.pixelSize: 20
                        font.bold: true
                    }
                    Rectangle {
                        radius: 10
                        color: "#12303a"
                        border.color: "#72d7cf"
                        Layout.preferredWidth: 80
                        Layout.preferredHeight: 20
                        Label {
                            anchors.centerIn: parent
                            text: "TK-O v0.2.0"
                            color: "#72d7cf"
                            font.pixelSize: 10
                            font.bold: true
                        }
                    }
                }
            }

            Rectangle { width: 1; height: 24; color: "#334155" }

            Label {
                text: bridge.currentView === "home" ? "Laboratório / Catálogo de Investigações"
                    : bridge.currentView === "wizard" ? "Laboratório / Nova Investigação Causal"
                    : "Laboratório / " + bridge.activeInvestigationId + " Workbench"
                color: "#94a3b8"
                font.pixelSize: 13
                Layout.fillWidth: true
            }

            PrimaryButton {
                text: "+ Nova Investigação"
                visible: bridge.currentView !== "wizard"
                onClicked: bridge.openWizard()
            }

            TkButton {
                text: "Voltar ao Catálogo"
                visible: bridge.currentView !== "home"
                onClicked: bridge.backToHome()
            }

            Rectangle {
                radius: 12
                color: "#232014"
                border.color: "#f3bf4f"
                Layout.preferredWidth: 145
                Layout.preferredHeight: 26
                RowLayout {
                    anchors.centerIn: parent
                    spacing: 6
                    Rectangle {
                        width: 7
                        height: 7
                        radius: 3.5
                        color: "#f3bf4f"
                        SequentialAnimation on opacity {
                            loops: Animation.Infinite
                            NumberAnimation { from: 0.4; to: 1.0; duration: 1000 }
                            NumberAnimation { from: 1.0; to: 0.4; duration: 1000 }
                        }
                    }
                    Label {
                        text: "READY ≠ COMPLETE"
                        color: "#f3bf4f"
                        font.pixelSize: 10
                        font.bold: true
                    }
                }
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 14
        spacing: 10

        // Status Message Strip
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 32
            radius: 6
            color: "#162332"
            border.color: "#283b4e"

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 12
                anchors.rightMargin: 12
                Label { text: "STATUS"; color: "#38bdf8"; font.bold: true; font.pixelSize: 11 }
                Label {
                    text: bridge.statusMessage
                    color: "#cbd5e1"
                    Layout.fillWidth: true
                    elide: Text.ElideRight
                    font.pixelSize: 11
                }
            }
        }

        // ========================================================
        // VIEW 1: LAB HOME (CATALOG OF INVESTIGATIONS)
        // ========================================================
        Item {
            visible: bridge.currentView === "home"
            Layout.fillWidth: true
            Layout.fillHeight: true

            ColumnLayout {
                anchors.fill: parent
                spacing: 14

                // Hero Card
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 120
                    radius: 12
                    color: "#111f30"
                    border.color: "#233950"

                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 20
                        spacing: 20

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 6
                            Label {
                                text: "Laboratório Experimental de Causalidade Mínima"
                                color: "#f8fafc"
                                font.pixelSize: 18
                                font.bold: true
                            }
                            Label {
                                text: "Descobrir, por construção, intervenção e comparação, quais estruturas são suficientes e quais relações são necessárias para a preservação de um fenômeno sob dado contexto."
                                color: "#94a3b8"
                                font.pixelSize: 12
                                wrapMode: Text.WordWrap
                                Layout.fillWidth: true
                            }
                        }

                        PrimaryButton {
                            text: "+ Formular Nova Investigação"
                            Layout.preferredWidth: 220
                            Layout.preferredHeight: 40
                            onClicked: bridge.openWizard()
                        }
                    }
                }

                // Global Stats Row
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 12

                    Panel {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 70
                        Column {
                            anchors.centerIn: parent
                            spacing: 4
                            Label { text: "INVESTIGAÇÕES NO WORKSPACE"; color: "#94a3b8"; font.pixelSize: 10; font.bold: true }
                            Label { text: bridge.globalStats.totalInvestigations || "2"; color: "#38bdf8"; font.pixelSize: 22; font.bold: true }
                        }
                    }

                    Panel {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 70
                        Column {
                            anchors.centerIn: parent
                            spacing: 4
                            Label { text: "EVIDÊNCIAS IMUTÁVEIS SHA-256"; color: "#94a3b8"; font.pixelSize: 10; font.bold: true }
                            Label { text: bridge.globalStats.totalEvidence || "20"; color: "#34d399"; font.pixelSize: 22; font.bold: true }
                        }
                    }

                    Panel {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 70
                        Column {
                            anchors.centerIn: parent
                            spacing: 4
                            Label { text: "CLAIMS SUSTENTADOS"; color: "#94a3b8"; font.pixelSize: 10; font.bold: true }
                            Label { text: bridge.globalStats.totalClaimsSupported || "3"; color: "#f3bf4f"; font.pixelSize: 22; font.bold: true }
                        }
                    }
                }

                // Catalog Grid
                SectionTitle { text: "CATÁLOGO DE INVESTIGAÇÕES ATIVAS" }

                GridView {
                    id: catalogGrid
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    cellWidth: 380
                    cellHeight: 190
                    clip: true
                    model: bridge.investigations

                    delegate: Rectangle {
                        required property var modelData
                        width: 360
                        height: 175
                        radius: 10
                        color: cardMouse.containsMouse ? "#182a3d" : "#132030"
                        border.color: cardMouse.containsMouse ? "#38bdf8" : "#24374b"
                        border.width: cardMouse.containsMouse ? 2 : 1

                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: 14
                            spacing: 8

                            RowLayout {
                                Layout.fillWidth: true
                                Label {
                                    text: modelData.id
                                    color: "#38bdf8"
                                    font.bold: true
                                    font.pixelSize: 14
                                    font.family: "monospace"
                                }
                                Item { Layout.fillWidth: true }
                                Rectangle {
                                    radius: 8
                                    color: modelData.isCanonical ? "#13353b" : (modelData.isSanity ? "#352c16" : (modelData.isBenchmark ? "#2e1065" : "#172d42"))
                                    border.color: modelData.isCanonical ? "#72d7cf" : (modelData.isSanity ? "#f3bf4f" : (modelData.isBenchmark ? "#c084fc" : "#38bdf8"))
                                    Layout.preferredWidth: modelData.isCanonical ? 130 : (modelData.isBenchmark ? 135 : 100)
                                    Layout.preferredHeight: 20
                                    Label {
                                        anchors.centerIn: parent
                                        text: modelData.isCanonical ? "Canônico • Referência" : (modelData.isSanity ? "Sanity Check" : (modelData.isBenchmark ? "Benchmark Territorial" : "Investigação Inédita"))
                                        color: modelData.isCanonical ? "#72d7cf" : (modelData.isSanity ? "#f3bf4f" : (modelData.isBenchmark ? "#c084fc" : "#38bdf8"))
                                        font.pixelSize: 9
                                        font.bold: true
                                    }
                                }
                            }

                            Label {
                                text: modelData.title
                                color: "#f8fafc"
                                font.bold: true
                                font.pixelSize: 13
                                elide: Text.ElideRight
                                Layout.fillWidth: true
                            }

                            Label {
                                text: modelData.phenomenonDesc
                                color: "#94a3b8"
                                font.pixelSize: 11
                                elide: Text.ElideRight
                                maximumLineCount: 2
                                wrapMode: Text.WordWrap
                                Layout.fillWidth: true
                            }

                            Rectangle { height: 1; Layout.fillWidth: true; color: "#223548" }

                            RowLayout {
                                Layout.fillWidth: true
                                Label {
                                    text: modelData.realizationsCount + " realizações • " + modelData.evidenceCount + " evidências"
                                    color: "#cbd5e1"
                                    font.pixelSize: 10
                                }
                                Item { Layout.fillWidth: true }
                                Label {
                                    text: "Abrir Workbench →"
                                    color: "#38bdf8"
                                    font.bold: true
                                    font.pixelSize: 11
                                }
                            }
                        }

                        MouseArea {
                            id: cardMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: bridge.openInvestigation(modelData.id)
                        }
                    }
                }
            }
        }

        // ========================================================
        // VIEW 2: WIZARD DE NOVA INVESTIGAÇÃO
        // ========================================================
        Item {
            visible: bridge.currentView === "wizard"
            Layout.fillWidth: true
            Layout.fillHeight: true

            Panel {
                anchors.centerIn: parent
                width: 760
                height: 540

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 24
                    spacing: 16

                    SectionTitle { text: "FORMULAR NOVA INVESTIGAÇÃO CAUSAL" }
                    Label {
                        text: "Defina uma nova pergunta científica causal sem pressupor componentes a priori."
                        color: "#94a3b8"
                        font.pixelSize: 12
                    }

                    Rectangle { height: 1; Layout.fillWidth: true; color: "#283b4e" }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 14
                        ColumnLayout {
                            Layout.fillWidth: true
                            Label { text: "IDENTIFICADOR"; color: "#72d7cf"; font.pixelSize: 10; font.bold: true }
                            TextField {
                                id: wizIdField
                                text: "TK-0002"
                                Layout.fillWidth: true
                                color: "#f8fafc"
                                background: Rectangle { color: "#0b1320"; border.color: "#334b63"; radius: 6 }
                            }
                        }
                        ColumnLayout {
                            Layout.fillWidth: true
                            Layout.preferredWidth: 300
                            Label { text: "NOME DO FENÔMENO"; color: "#72d7cf"; font.pixelSize: 10; font.bold: true }
                            TextField {
                                id: wizNameField
                                placeholderText: "Ex: regulação homeostática / filtro de ruído"
                                Layout.fillWidth: true
                                color: "#f8fafc"
                                background: Rectangle { color: "#0b1320"; border.color: "#334b63"; radius: 6 }
                            }
                        }
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        Label { text: "DESCRIÇÃO DO FENÔMENO"; color: "#72d7cf"; font.pixelSize: 10; font.bold: true }
                        TextArea {
                            id: wizDescField
                            placeholderText: "O que caracteriza a ocorrência do fenômeno sob observação..."
                            Layout.fillWidth: true
                            Layout.preferredHeight: 60
                            color: "#f8fafc"
                            wrapMode: Text.WordWrap
                            background: Rectangle { color: "#0b1320"; border.color: "#334b63"; radius: 6 }
                        }
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        Label { text: "COMPONENTES DA BASELINE INICIAL (separados por vírgula)"; color: "#72d7cf"; font.pixelSize: 10; font.bold: true }
                        TextField {
                            id: wizCompsField
                            text: "sensor, filtro, integrador, atuador"
                            Layout.fillWidth: true
                            color: "#f8fafc"
                            background: Rectangle { color: "#0b1320"; border.color: "#334b63"; radius: 6 }
                        }
                    }

                    Item { Layout.fillHeight: true }

                    RowLayout {
                        Layout.fillWidth: true
                        TkButton {
                            text: "Cancelar"
                            onClicked: bridge.backToHome()
                        }
                        Item { Layout.fillWidth: true }
                        PrimaryButton {
                            text: "Materializar e Abrir Investigação"
                            onClicked: {
                                const comps = wizCompsField.text.split(",").map(s => s.trim()).filter(s => s.length > 0)
                                bridge.createInvestigation({
                                    id: wizIdField.text,
                                    phenomenonName: wizNameField.text,
                                    phenomenonDesc: wizDescField.text,
                                    baselineComponents: comps
                                })
                            }
                        }
                    }
                }
            }
        }

        // ========================================================
        // VIEW 3: WORKBENCH ANALÍTICO
        // ========================================================
        Item {
            visible: bridge.currentView === "workbench"
            Layout.fillWidth: true
            Layout.fillHeight: true

            ColumnLayout {
                anchors.fill: parent
                spacing: 10

                // Workbench Action Toolbar
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 10

                    Label {
                        text: bridge.activeInvestigationId + " — Workbench Analítico"
                        color: "#f8fafc"
                        font.pixelSize: 16
                        font.bold: true
                        Layout.fillWidth: true
                    }

                    TkButton {
                        text: "🔬 + Registrar Observação"
                        onClicked: dialogObservation.open()
                    }

                    PrimaryButton {
                        text: "+ Nova Intervenção"
                        onClicked: dialogIntervention.open()
                    }

                    TkButton {
                        text: "⚖ Adjudicar Witnesses"
                        onClicked: bridge.adjudicateWitnesses()
                    }

                    TkButton {
                        text: "Exportar JSON"
                        onClicked: bridge.exportCurrent()
                    }
                }

                // Workbench 3-Column Top Grid
                RowLayout {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.minimumHeight: 400
                    spacing: 12

                    // Left: Phenomenon, Context & Profile
                    Panel {
                        Layout.preferredWidth: 320
                        Layout.fillHeight: true

                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: 14
                            spacing: 8

                            SectionTitle { text: "FENÔMENO / PERFIL CONSTITUTIVO" }
                            Label {
                                text: bridge.phenomenon
                                color: "#f8fafc"
                                wrapMode: Text.WordWrap
                                Layout.fillWidth: true
                                font.pixelSize: 12
                            }
                            Rectangle { color: "#283b4e"; height: 1; Layout.fillWidth: true }
                            Label {
                                text: bridge.context
                                color: "#b9c7d4"
                                wrapMode: Text.WordWrap
                                Layout.fillWidth: true
                                font.pixelSize: 11
                            }
                            ScrollView {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                clip: true
                                Label {
                                    text: bridge.profile
                                    color: "#94a3b8"
                                    wrapMode: Text.WordWrap
                                    width: 280
                                    font.pixelSize: 11
                                }
                            }
                        }
                    }

                    // Center: Causal Space Graph
                    Panel {
                        Layout.fillWidth: true
                        Layout.minimumWidth: 540
                        Layout.fillHeight: true

                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: 14
                            spacing: 6

                            RowLayout {
                                Layout.fillWidth: true
                                SectionTitle { text: "ESPAÇO CAUSAL  Gₚ = (R, I)"; Layout.fillWidth: true }
                                Label { text: "clique nos nós para inspecionar"; color: "#8fa4b6"; font.pixelSize: 11 }
                            }

                            Item {
                                id: graphArea
                                Layout.fillWidth: true
                                Layout.fillHeight: true

                                Canvas {
                                    id: edges
                                    anchors.fill: parent
                                    z: 0
                                    onPaint: {
                                        const context = getContext("2d")
                                        context.reset()
                                        context.lineWidth = 2
                                        context.strokeStyle = "#38bdf8"
                                        context.fillStyle = "#38bdf8"
                                        const nodes = bridge.realizations
                                        const itvs = bridge.interventions
                                        for (let i = 0; i < itvs.length; ++i) {
                                            const edge = itvs[i]
                                            if (edge.status !== "performed") continue
                                            let src = null, tgt = null
                                            for (let j = 0; j < nodes.length; ++j) {
                                                if (nodes[j].id === edge.source) src = nodes[j]
                                                if (nodes[j].id === edge.target) tgt = nodes[j]
                                            }
                                            if (!src || !tgt) continue
                                            const sx = src.x + 190, sy = src.y + 36
                                            const tx = tgt.x, ty = tgt.y + 36

                                            context.beginPath()
                                            context.moveTo(sx, sy)
                                            context.bezierCurveTo(sx + 50, sy, tx - 50, ty, tx - 8, ty)
                                            context.stroke()

                                            context.beginPath()
                                            context.moveTo(tx, ty)
                                            context.lineTo(tx - 10, ty - 5)
                                            context.lineTo(tx - 10, ty + 5)
                                            context.closePath()
                                            context.fill()
                                        }
                                    }
                                    Connections {
                                        target: bridge
                                        function onDataChanged() { edges.requestPaint() }
                                    }
                                }

                                // Intervention Action Badges
                                Repeater {
                                    model: bridge.interventions
                                    delegate: TkButton {
                                        required property var modelData
                                        visible: modelData.status === "performed"
                                        x: modelData.x
                                        y: modelData.y
                                        z: 2
                                        width: 140
                                        height: 28
                                        text: modelData.kind
                                        font.pixelSize: 11
                                        ToolTip.visible: hovered
                                        ToolTip.text: modelData.id + "\nPredição: " + modelData.prediction
                                        onClicked: bridge.selectEntity(modelData.id)
                                    }
                                }

                                // Realization Nodes
                                Repeater {
                                    model: bridge.realizations
                                    delegate: Rectangle {
                                        required property var modelData
                                        x: modelData.x
                                        y: modelData.y
                                        z: 3
                                        width: 190
                                        height: 72
                                        radius: 9
                                        color: modelData.outcome === "preserving" ? "#1d5041"
                                              : modelData.outcome === "ruptured" ? "#653938" : "#24374b"
                                        border.color: modelData.outcome === "preserving" ? "#34d399"
                                                    : modelData.outcome === "ruptured" ? "#f87171" : "#38bdf8"
                                        border.width: 1

                                        Column {
                                            anchors.centerIn: parent
                                            width: parent.width - 16
                                            spacing: 4
                                            Label {
                                                text: modelData.label
                                                color: "white"
                                                font.bold: true
                                                width: parent.width
                                                elide: Text.ElideRight
                                                horizontalAlignment: Text.AlignHCenter
                                            }
                                            Label {
                                                text: modelData.outcome.toUpperCase() + " • |Σ|=" + modelData.componentsCount
                                                color: "#cbd5e1"
                                                width: parent.width
                                                horizontalAlignment: Text.AlignHCenter
                                                font.pixelSize: 10
                                            }
                                        }
                                        MouseArea {
                                            anchors.fill: parent
                                            cursorShape: Qt.PointingHandCursor
                                            onClicked: bridge.selectEntity(modelData.id)
                                        }
                                    }
                                }
                            }
                        }
                    }

                    // Right: Runs & Evidence
                    Panel {
                        Layout.preferredWidth: 350
                        Layout.fillHeight: true

                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: 14
                            spacing: 8

                            SectionTitle { text: "RUNS / EVIDÊNCIA SHA-256" }
                            ListView {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 160
                                clip: true
                                model: bridge.runs
                                spacing: 6

                                delegate: Rectangle {
                                    required property var modelData
                                    width: ListView.view.width
                                    height: 48
                                    radius: 6
                                    color: runMouse.containsMouse ? "#1c3247" : "#142232"
                                    border.color: "#283b4e"

                                    Column {
                                        anchors.fill: parent
                                        anchors.margins: 6
                                        Label { text: modelData.id; color: "#f8fafc"; font.pixelSize: 11; font.bold: true }
                                        Label {
                                            text: modelData.evidenceCount + " evidências imutáveis • " + modelData.result
                                            color: "#94a3b8"
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

                            SectionTitle { text: "INSPEÇÃO & PROVENIÊNCIA" }
                            ScrollView {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                clip: true
                                Label {
                                    text: bridge.selectedDetails
                                    color: "#cbd5e1"
                                    wrapMode: Text.Wrap
                                    width: 310
                                    font.pixelSize: 11
                                }
                            }
                        }
                    }
                }

                // Bottom 2-Column Grid: Claims & Frontier
                RowLayout {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 200
                    spacing: 12

                    Panel {
                        Layout.preferredWidth: 540
                        Layout.fillHeight: true

                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: 14
                            SectionTitle { text: "ESCADA DE CLAIMS (L0–L8)" }
                            ListView {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                model: bridge.claims
                                spacing: 4
                                clip: true

                                delegate: Rectangle {
                                    required property var modelData
                                    width: ListView.view.width
                                    height: 38
                                    radius: 6
                                    color: "#142232"

                                    RowLayout {
                                        anchors.fill: parent
                                        anchors.margins: 6
                                        Rectangle {
                                            radius: 4
                                            color: "#352c16"
                                            border.color: "#f3bf4f"
                                            Layout.preferredWidth: 32
                                            Layout.preferredHeight: 22
                                            Label {
                                                anchors.centerIn: parent
                                                text: modelData.level
                                                color: "#f3bf4f"
                                                font.bold: true
                                                font.pixelSize: 10
                                            }
                                        }
                                        Label {
                                            text: modelData.assertion
                                            color: "#f8fafc"
                                            Layout.fillWidth: true
                                            elide: Text.ElideRight
                                            font.pixelSize: 11
                                        }
                                        Label {
                                            text: modelData.status.toUpperCase()
                                            color: modelData.status === "supported" ? "#34d399" : "#f3bf4f"
                                            font.bold: true
                                            font.pixelSize: 10
                                        }
                                    }
                                    MouseArea {
                                        anchors.fill: parent
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
                            SectionTitle { text: "FRONTEIRA & INCOMPLETUDE" }
                            Label {
                                text: bridge.frontier
                                color: "#f8fafc"
                                wrapMode: Text.WordWrap
                                Layout.fillWidth: true
                                font.pixelSize: 11
                            }
                            Rectangle { color: "#283b4e"; height: 1; Layout.fillWidth: true }
                            Label {
                                text: "As candidatas são minimais apenas sob a ordem Gamma declarada. Intervenções abertas impedem saltos indutivos universais."
                                color: "#94a3b8"
                                wrapMode: Text.WordWrap
                                Layout.fillWidth: true
                                font.pixelSize: 10
                            }
                            Item { Layout.fillHeight: true }
                            Label { text: "INCOMPLETE BY DESIGN"; color: "#f3bf4f"; font.bold: true; font.pixelSize: 11 }
                        }
                    }
                }
            }
        }
    }

    // Dialog Modal: Adicionar Intervenção
    Dialog {
        id: dialogIntervention
        title: "Adicionar Intervenção Causal"
        anchors.centerIn: parent
        width: 440
        modal: true
        standardButtons: Dialog.Ok | Dialog.Cancel

        background: Rectangle {
            color: "#0f172a"
            border.color: "#38bdf8"
            radius: 10
        }

        ColumnLayout {
            anchors.fill: parent
            spacing: 12

            Label { text: "Operador de Intervenção:"; color: "#72d7cf"; font.bold: true }
            ComboBox {
                id: comboKind
                model: ["remove", "replace", "disable", "perturb"]
                Layout.fillWidth: true
            }

            Label { text: "Componente Alvo:"; color: "#72d7cf"; font.bold: true }
            TextField {
                id: itvTargetField
                placeholderText: "Ex: feedback, update, sensor, filtro..."
                Layout.fillWidth: true
                color: "#f8fafc"
                background: Rectangle { color: "#1e293b"; radius: 6 }
            }

            Label {
                text: "Componente Substituto (para 'replace'):"
                color: "#72d7cf"
                font.bold: true
                visible: comboKind.currentText === "replace"
            }
            TextField {
                id: itvReplField
                placeholderText: "Ex: feedback_equivalent"
                Layout.fillWidth: true
                color: "#f8fafc"
                visible: comboKind.currentText === "replace"
                background: Rectangle { color: "#1e293b"; radius: 6 }
            }
        }

        onAccepted: {
            bridge.addIntervention("TK-0001:R:BASE", comboKind.currentText, itvTargetField.text, itvReplField.text)
        }
    }

    // Dialog Modal: Registrar Observação Empírica
    Dialog {
        id: dialogObservation
        title: "Registrar Observação Empírica de Campo"
        anchors.centerIn: parent
        width: 460
        modal: true
        standardButtons: Dialog.Ok | Dialog.Cancel

        background: Rectangle {
            color: "#0f172a"
            border.color: "#34d399"
            radius: 10
        }

        ColumnLayout {
            anchors.fill: parent
            spacing: 12

            Label { text: "Dimensão Constitutiva:"; color: "#34d399"; font.bold: true }
            ComboBox {
                id: comboObsDimension
                model: ["operational", "causal", "discriminative", "observational", "temporal"]
                Layout.fillWidth: true
            }

            Label { text: "Status da Medição / Witness:"; color: "#34d399"; font.bold: true }
            ComboBox {
                id: comboObsStatus
                model: ["Satisfeito (Preserved)", "Não Satisfeito (Broken)"]
                Layout.fillWidth: true
            }

            Label { text: "Traço de Medição / Log:"; color: "#34d399"; font.bold: true }
            TextField {
                id: obsTraceField
                placeholderText: "Ex: medicao_biomassa=42kg; status=preservado"
                Layout.fillWidth: true
                color: "#f8fafc"
                background: Rectangle { color: "#1e293b"; radius: 6 }
            }
        }

        onAccepted: {
            const isSat = comboObsStatus.currentIndex === 0
            bridge.injectObservation("", comboObsDimension.currentText, isSat, obsTraceField.text)
        }
    }
}
