#include <QApplication>
#include <QMainWindow>
#include <QWidget>
#include <QPainter>
#include <QImage>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QActionGroup>
#include <QTimer>
#include <QKeyEvent>
#include <QFileDialog>
#include <QMessageBox>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QStatusBar>
#include <QFileInfo>
#include <QElapsedTimer>
#include "chip8.h"

// Color Palette Struct
struct QtColorPalette {
    QColor bg;
    QColor fg;
    const char* name;
};

static const QtColorPalette QT_PALETTES[] = {
    { QColor(15,  15,  20),  QColor(240, 240, 245), "Classic White" },
    { QColor(10,  26,  12),  QColor(57,  255, 20),  "Green Phosphor" },
    { QColor(28,  16,  0),   QColor(255, 176, 0),   "Amber CRT" },
    { QColor(18,  9,   36),  QColor(0,   240, 255), "Cyberpunk Cyan" }
};

// CHIP-8 Opcode Disassembler Helper
static QString DisassembleOpcode(unsigned short opcode) {
    unsigned short nnn = opcode & 0x0FFF;
    unsigned char n = opcode & 0x000F;
    unsigned char x = (opcode & 0x0F00) >> 8;
    unsigned char y = (opcode & 0x00F0) >> 4;
    unsigned char kk = opcode & 0x00FF;

    switch (opcode & 0xF000) {
        case 0x0000:
            if (opcode == 0x00E0) return "CLS (Clear Display)";
            if (opcode == 0x00EE) return "RET (Return Subroutine)";
            return QString("SYS 0x%1").arg(nnn, 3, 16, QChar('0')).toUpper();

        case 0x1000: return QString("JP 0x%1").arg(nnn, 3, 16, QChar('0')).toUpper();
        case 0x2000: return QString("CALL 0x%1").arg(nnn, 3, 16, QChar('0')).toUpper();
        case 0x3000: return QString("SE V%1, 0x%2").arg(QString::number(x, 16).toUpper()).arg(kk, 2, 16, QChar('0')).toUpper();
        case 0x4000: return QString("SNE V%1, 0x%2").arg(QString::number(x, 16).toUpper()).arg(kk, 2, 16, QChar('0')).toUpper();
        case 0x5000: return QString("SE V%1, V%2").arg(QString::number(x, 16).toUpper()).arg(QString::number(y, 16).toUpper());
        case 0x6000: return QString("LD V%1, 0x%2").arg(QString::number(x, 16).toUpper()).arg(kk, 2, 16, QChar('0')).toUpper();
        case 0x7000: return QString("ADD V%1, 0x%2").arg(QString::number(x, 16).toUpper()).arg(kk, 2, 16, QChar('0')).toUpper();

        case 0x8000:
            switch (n) {
                case 0x0: return QString("LD V%1, V%2").arg(QString::number(x, 16).toUpper()).arg(QString::number(y, 16).toUpper());
                case 0x1: return QString("OR V%1, V%2").arg(QString::number(x, 16).toUpper()).arg(QString::number(y, 16).toUpper());
                case 0x2: return QString("AND V%1, V%2").arg(QString::number(x, 16).toUpper()).arg(QString::number(y, 16).toUpper());
                case 0x3: return QString("XOR V%1, V%2").arg(QString::number(x, 16).toUpper()).arg(QString::number(y, 16).toUpper());
                case 0x4: return QString("ADD V%1, V%2").arg(QString::number(x, 16).toUpper()).arg(QString::number(y, 16).toUpper());
                case 0x5: return QString("SUB V%1, V%2").arg(QString::number(x, 16).toUpper()).arg(QString::number(y, 16).toUpper());
                case 0x6: return QString("SHR V%1").arg(QString::number(x, 16).toUpper());
                case 0x7: return QString("SUBN V%1, V%2").arg(QString::number(x, 16).toUpper()).arg(QString::number(y, 16).toUpper());
                case 0xE: return QString("SHL V%1").arg(QString::number(x, 16).toUpper());
            }
            break;

        case 0x9000: return QString("SNE V%1, V%2").arg(QString::number(x, 16).toUpper()).arg(QString::number(y, 16).toUpper());
        case 0xA000: return QString("LD I, 0x%1").arg(nnn, 3, 16, QChar('0')).toUpper();
        case 0xB000: return QString("JP V0, 0x%1").arg(nnn, 3, 16, QChar('0')).toUpper();
        case 0xC000: return QString("RND V%1, 0x%2").arg(QString::number(x, 16).toUpper()).arg(kk, 2, 16, QChar('0')).toUpper();
        case 0xD000: return QString("DRW V%1, V%2, %3").arg(QString::number(x, 16).toUpper()).arg(QString::number(y, 16).toUpper()).arg(n);

        case 0xE000:
            if (kk == 0x9E) return QString("SKP V%1").arg(QString::number(x, 16).toUpper());
            if (kk == 0xA1) return QString("SKNP V%1").arg(QString::number(x, 16).toUpper());
            break;

        case 0xF000:
            switch (kk) {
                case 0x07: return QString("LD V%1, DT").arg(QString::number(x, 16).toUpper());
                case 0x0A: return QString("LD V%1, K").arg(QString::number(x, 16).toUpper());
                case 0x15: return QString("LD DT, V%1").arg(QString::number(x, 16).toUpper());
                case 0x18: return QString("LD ST, V%1").arg(QString::number(x, 16).toUpper());
                case 0x1E: return QString("ADD I, V%1").arg(QString::number(x, 16).toUpper());
                case 0x29: return QString("LD F, V%1").arg(QString::number(x, 16).toUpper());
                case 0x33: return QString("LD BCD, V%1").arg(QString::number(x, 16).toUpper());
                case 0x55: return QString("LD [I], V%1").arg(QString::number(x, 16).toUpper());
                case 0x65: return QString("LD V%1, [I]").arg(QString::number(x, 16).toUpper());
            }
            break;
    }
    return QString("UNK 0x%1").arg(opcode, 4, 16, QChar('0')).toUpper();
}

// Canvas Widget for CHIP-8 Display
class Chip8Canvas : public QWidget {
public:
    chip8* m_chip8 = nullptr;
    bool m_romLoaded = false;
    bool m_isPaused = false;
    int m_paletteIndex = 0;
    QImage m_image;

    Chip8Canvas(chip8* c8, QWidget* parent = nullptr)
        : QWidget(parent), m_chip8(c8), m_image(64, 32, QImage::Format_RGB32)
    {
        setAcceptDrops(true);
        setFocusPolicy(Qt::StrongFocus);
    }

protected:
    void paintEvent(QPaintEvent* event) override {
        Q_UNUSED(event);
        QPainter painter(this);

        const QtColorPalette& pal = QT_PALETTES[m_paletteIndex];

        if (!m_romLoaded) {
            // Fill Background
            painter.fillRect(rect(), pal.bg);

            // Draw Welcome Text
            painter.setPen(pal.fg);
            QFont titleFont("Segoe UI", 20, QFont::Bold);
            painter.setFont(titleFont);
            painter.drawText(rect().adjusted(0, -60, 0, 0), Qt::AlignCenter, "CHIP-8 EMULATOR (Qt)");

            QFont subFont("Segoe UI", 11);
            painter.setFont(subFont);
            painter.drawText(rect().adjusted(0, 20, 0, 0), Qt::AlignCenter,
                             "Click File -> Open ROM (Ctrl+O) or Drag & Drop a .ch8 file");
            painter.drawText(rect().adjusted(0, 60, 0, 0), Qt::AlignCenter,
                             "Keypad Mapping: 1-4, Q-R, A-F, Z-V");
            return;
        }

        // Render CHIP-8 64x32 buffer into QImage
        QRgb fgRgb = pal.fg.rgb();
        QRgb bgRgb = pal.bg.rgb();

        for (int i = 0; i < 2048; ++i) {
            int x = i % 64;
            int y = i / 64;
            m_image.setPixel(x, y, (m_chip8->gfx[i] != 0) ? fgRgb : bgRgb);
        }

        // Scale 64x32 QImage to canvas size using nearest-neighbor (crisp pixels)
        painter.setRenderHint(QPainter::SmoothPixmapTransform, false);
        painter.drawImage(rect(), m_image);

        // Pause Overlay Indicator
        if (m_isPaused) {
            painter.setPen(QColor(255, 70, 70));
            QFont pauseFont("Segoe UI", 14, QFont::Bold);
            painter.setFont(pauseFont);
            painter.drawText(15, 30, "[PAUSED - Press F10 to Step]");
        }

        // Live Opcode & Register Debug HUD Banner
        if (m_chip8 && m_romLoaded) {
            unsigned short pc = m_chip8->pc;
            if (pc < 4095) {
                unsigned short op = (m_chip8->memory[pc] << 8) | m_chip8->memory[pc + 1];
                QString disasm = DisassembleOpcode(op);

                int hudHeight = 36;
                QRect hudRect(10, height() - hudHeight - 10, width() - 20, hudHeight);

                // Semi-transparent dark overlay container with rounded border
                painter.setPen(QColor(0, 240, 255, 180));
                painter.setBrush(QColor(10, 10, 18, 220));
                painter.drawRoundedRect(hudRect, 6, 6);

                painter.setPen(QColor(240, 240, 245));
                QFont debugFont("Consolas", 11, QFont::Bold);
                painter.setFont(debugFont);

                QString debugText = QString("PC: 0x%1  |  NEXT OP: 0x%2 -> %3  |  I: 0x%4  |  SP: %5")
                    .arg(pc, 4, 16, QChar('0')).toUpper()
                    .arg(op, 4, 16, QChar('0')).toUpper()
                    .arg(disasm)
                    .arg(m_chip8->I, 4, 16, QChar('0')).toUpper()
                    .arg(m_chip8->sp);

                painter.drawText(hudRect, Qt::AlignCenter, debugText);
            }
        }
    }
};

// Main Window Class
class MainWindow : public QMainWindow {
    chip8 m_chip8;
    Chip8Canvas* m_canvas = nullptr;

    QTimer* m_timer60Hz = nullptr;
    QTimer* m_cpuTimer = nullptr;

    QString m_currentRomPath;
    int m_targetHz = 500;
    bool m_pauseOnStart = false;

    QAction* m_actionPause = nullptr;
    QAction* m_speedActions[4];
    QAction* m_paletteActions[4];

public:
    MainWindow(QWidget* parent = nullptr) : QMainWindow(parent) {
        setWindowTitle("CHIP-8 Emulator - [No ROM Loaded]");
        resize(768, 384 + 25); // 64x32 scaled 12x plus status bar

        m_chip8.initialize();

        m_canvas = new Chip8Canvas(&m_chip8, this);
        setCentralWidget(m_canvas);

        setupMenus();
        setupTimers();
        applyDarkStyleSheet();

        setAcceptDrops(true);
        m_canvas->setFocus();
    }

private:
    int mapQtKeyToChip8(int key) {
        switch (key) {
            case Qt::Key_1: return 0x1;
            case Qt::Key_2: return 0x2;
            case Qt::Key_3: return 0x3;
            case Qt::Key_4: return 0xC;

            case Qt::Key_Q: return 0x4;
            case Qt::Key_W: return 0x5;
            case Qt::Key_E: return 0x6;
            case Qt::Key_R: return 0x7;

            case Qt::Key_A: return 0x8;
            case Qt::Key_S: return 0x9;
            case Qt::Key_D: return 0xE;
            case Qt::Key_F: return 0xE;

            case Qt::Key_Z: return 0xA;
            case Qt::Key_X: return 0x0;
            case Qt::Key_C: return 0xB;
            case Qt::Key_V: return 0xF;

            default: return -1;
        }
    }

protected:
    void keyPressEvent(QKeyEvent* event) override {
        if (event->key() == Qt::Key_O && (event->modifiers() & Qt::ControlModifier)) {
            openRomDialog();
            return;
        }
        if (event->key() == Qt::Key_R && (event->modifiers() & Qt::ControlModifier)) {
            resetRom();
            return;
        }
        if (event->key() == Qt::Key_Space) {
            togglePause();
            return;
        }

        int chip8Key = mapQtKeyToChip8(event->key());
        if (chip8Key >= 0 && chip8Key < 16) {
            m_chip8.key[chip8Key] = 1;
        } else {
            QMainWindow::keyPressEvent(event);
        }
    }

    void keyReleaseEvent(QKeyEvent* event) override {
        int chip8Key = mapQtKeyToChip8(event->key());
        if (chip8Key >= 0 && chip8Key < 16) {
            m_chip8.key[chip8Key] = 0;
        } else {
            QMainWindow::keyReleaseEvent(event);
        }
    }

    void dragEnterEvent(QDragEnterEvent* event) override {
        if (event->mimeData()->hasUrls()) {
            event->acceptProposedAction();
        }
    }

    void dropEvent(QDropEvent* event) override {
        const QList<QUrl> urls = event->mimeData()->urls();
        if (!urls.isEmpty()) {
            QString localPath = urls.first().toLocalFile();
            if (!localPath.isEmpty()) {
                loadRom(localPath);
            }
        }
    }

private:
    void setupMenus() {
        QMenuBar* bar = menuBar();

        // File Menu
        QMenu* fileMenu = bar->addMenu("&File");
        QAction* actOpen = fileMenu->addAction("&Open ROM...", this, &MainWindow::openRomDialog, QKeySequence("Ctrl+O"));
        QAction* actReset = fileMenu->addAction("&Reset ROM", this, &MainWindow::resetRom, QKeySequence("Ctrl+R"));
        fileMenu->addSeparator();
        fileMenu->addAction("E&xit", this, &QWidget::close);

        // Emulation Menu
        QMenu* emuMenu = bar->addMenu("&Emulation");
        m_actionPause = emuMenu->addAction("&Pause / Resume", this, &MainWindow::togglePause, QKeySequence("Space"));
        emuMenu->addAction("&Step Single Cycle", this, &MainWindow::stepCycle, QKeySequence("F10"));
        emuMenu->addSeparator();

        QAction* actPauseOnStart = emuMenu->addAction("Pause on ROM Load");
        actPauseOnStart->setCheckable(true);
        actPauseOnStart->setChecked(m_pauseOnStart);
        connect(actPauseOnStart, &QAction::toggled, this, [this](bool checked) {
            m_pauseOnStart = checked;
            statusBar()->showMessage(m_pauseOnStart ? "Pause on ROM Load Enabled" : "Pause on ROM Load Disabled", 2000);
        });

        // Speed Menu
        QMenu* speedMenu = bar->addMenu("&Speed");
        int speeds[4] = { 300, 500, 800, 1200 };
        const char* speedNames[4] = { "300 Hz (Slow)", "500 Hz (Normal)", "800 Hz (Fast)", "1200 Hz (Turbo)" };

        QActionGroup* speedGroup = new QActionGroup(this);
        for (int i = 0; i < 4; ++i) {
            m_speedActions[i] = speedMenu->addAction(speedNames[i]);
            m_speedActions[i]->setCheckable(true);
            speedGroup->addAction(m_speedActions[i]);

            if (speeds[i] == m_targetHz) m_speedActions[i]->setChecked(true);

            connect(m_speedActions[i], &QAction::triggered, this, [this, speeds, i]() {
                m_targetHz = speeds[i];
                m_cpuTimer->setInterval(1000 / m_targetHz);
                statusBar()->showMessage(QString("CPU Frequency set to %1 Hz").arg(m_targetHz), 2000);
            });
        }

        // Palette Menu
        QMenu* palMenu = bar->addMenu("&Palette");
        QActionGroup* palGroup = new QActionGroup(this);
        for (int i = 0; i < 4; ++i) {
            m_paletteActions[i] = palMenu->addAction(QT_PALETTES[i].name);
            m_paletteActions[i]->setCheckable(true);
            palGroup->addAction(m_paletteActions[i]);

            if (i == 0) m_paletteActions[i]->setChecked(true);

            connect(m_paletteActions[i], &QAction::triggered, this, [this, i]() {
                m_canvas->m_paletteIndex = i;
                m_canvas->update();
            });
        }

        // Help Menu
        QMenu* helpMenu = bar->addMenu("&Help");
        helpMenu->addAction("&Keypad Guide", this, &MainWindow::showControlsGuide);
        helpMenu->addAction("&About", this, &MainWindow::showAbout);

        statusBar()->showMessage("Ready. Load a ROM to start.");
    }

    void setupTimers() {
        // 60 Hz timer for CHIP-8 delay and sound timers
        m_timer60Hz = new QTimer(this);
        connect(m_timer60Hz, &QTimer::timeout, this, [this]() {
            if (m_canvas->m_romLoaded && !m_canvas->m_isPaused) {
                if (m_chip8.delay_timer > 0) --m_chip8.delay_timer;
                if (m_chip8.sound_timer > 0) --m_chip8.sound_timer;
            }
        });
        m_timer60Hz->start(1000 / 60);

        // CPU Execution Timer
        m_cpuTimer = new QTimer(this);
        connect(m_cpuTimer, &QTimer::timeout, this, [this]() {
            if (m_canvas->m_romLoaded && !m_canvas->m_isPaused) {
                try {
                    m_chip8.emulateCycle();
                    updateDisassemblerStatus();
                    if (m_chip8.drawFlag) {
                        m_canvas->update();
                        m_chip8.drawFlag = false;
                    }
                } catch (const std::exception& e) {
                    m_canvas->m_isPaused = true;
                    statusBar()->showMessage(QString("Emulation paused: %1").arg(e.what()));
                } catch (...) {
                    m_canvas->m_isPaused = true;
                    statusBar()->showMessage("Emulation paused: Opcode execution fault caught.");
                }
            }
        });
        m_cpuTimer->start(1000 / m_targetHz);
    }

    void openRomDialog() {
        QString fileName = QFileDialog::getOpenFileName(
            this,
            "Select CHIP-8 ROM File",
            QString(),
            "CHIP-8 ROM Files (*.ch8 *.rom *.bin);;All Files (*.*)"
        );

        if (!fileName.isEmpty()) {
            loadRom(fileName);
        }
    }

    void updateDisassemblerStatus() {
        if (!m_canvas->m_romLoaded) return;

        unsigned short pc = m_chip8.pc;
        if (pc < 4095) {
            unsigned short op = (m_chip8.memory[pc] << 8) | m_chip8.memory[pc + 1];
            QString disasm = DisassembleOpcode(op);

            QString statusText = QString("[%1] PC: 0x%2 | Next Op: 0x%3 -> %4 | I: 0x%5 | SP: %6 | V0: 0x%7 VF: 0x%8")
                .arg(m_canvas->m_isPaused ? "PAUSED" : "RUNNING")
                .arg(pc, 4, 16, QChar('0')).toUpper()
                .arg(op, 4, 16, QChar('0')).toUpper()
                .arg(disasm)
                .arg(m_chip8.I, 4, 16, QChar('0')).toUpper()
                .arg(m_chip8.sp)
                .arg(m_chip8.V[0], 2, 16, QChar('0')).toUpper()
                .arg(m_chip8.V[0xF], 2, 16, QChar('0')).toUpper();

            statusBar()->showMessage(statusText);
        }
    }

    void loadRom(const QString& path) {
        m_chip8.initialize();
        m_chip8.loadGame(path.toUtf8().constData());

        m_currentRomPath = path;
        m_canvas->m_romLoaded = true;
        m_canvas->m_isPaused = m_pauseOnStart;

        QFileInfo info(path);
        setWindowTitle(QString("CHIP-8 Emulator - [%1]").arg(info.fileName()));

        updateDisassemblerStatus();
        m_canvas->update();
    }

    void resetRom() {
        if (!m_currentRomPath.isEmpty()) {
            loadRom(m_currentRomPath);
        } else {
            openRomDialog();
        }
    }

    void togglePause() {
        if (m_canvas->m_romLoaded) {
            m_canvas->m_isPaused = !m_canvas->m_isPaused;
            updateDisassemblerStatus();
            m_canvas->update();
        }
    }

    void stepCycle() {
        if (m_canvas->m_romLoaded) {
            try {
                m_chip8.emulateCycle();
                updateDisassemblerStatus();
                m_canvas->update();
            } catch (...) {
                m_canvas->m_isPaused = true;
                statusBar()->showMessage("Opcode fault during step.");
            }
        }
    }

    void showControlsGuide() {
        QMessageBox::information(
            this,
            "Keypad Controls Guide",
            "CHIP-8 Keypad to PC Keyboard Mapping:\n\n"
            "CHIP-8 Keypad:          PC Keyboard:\n"
            "  [1] [2] [3] [C]   ->    [1] [2] [3] [4]\n"
            "  [4] [5] [6] [D]   ->    [Q] [W] [E] [R]\n"
            "  [7] [8] [9] [E]   ->    [A] [S] [D] [F]\n"
            "  [A] [0] [B] [F]   ->    [Z] [X] [C] [V]\n\n"
            "Shortcuts:\n"
            "  Ctrl+O : Open ROM file\n"
            "  Ctrl+R : Reset current ROM\n"
            "  Space  : Pause / Resume\n"
            "  F10    : Step single cycle"
        );
    }

    void showAbout() {
        QMessageBox::about(
            this,
            "About CHIP-8 Emulator (Qt)",
            "CHIP-8 Emulator in C++ & Qt6 / Qt5\n\n"
            "Cross-platform Qt UI layer over CHIP-8 Interpreter Core.\n"
            "Features: Drag & Drop, Color Palettes, Speed Control."
        );
    }

    void applyDarkStyleSheet() {
        setStyleSheet(R"(
            QMainWindow {
                background-color: #1e1e2e;
            }
            QMenuBar {
                background-color: #181825;
                color: #cdd6f4;
                border-bottom: 1px solid #313244;
                padding: 4px;
                font-family: 'Segoe UI', sans-serif;
                font-size: 13px;
            }
            QMenuBar::item {
                background: transparent;
                padding: 4px 10px;
                border-radius: 4px;
            }
            QMenuBar::item:selected {
                background-color: #313244;
                color: #cdd6f4;
            }
            QMenu {
                background-color: #181825;
                color: #cdd6f4;
                border: 1px solid #45475a;
                padding: 4px;
                font-size: 13px;
            }
            QMenu::item {
                padding: 6px 24px 6px 12px;
                border-radius: 4px;
            }
            QMenu::item:selected {
                background-color: #45475a;
                color: #ffffff;
            }
            QMenu::separator {
                height: 1px;
                background-color: #313244;
                margin: 4px 0px;
            }
            QStatusBar {
                background-color: #11111b;
                color: #a6adc8;
                font-size: 12px;
                border-top: 1px solid #313244;
            }
            QMessageBox {
                background-color: #1e1e2e;
                color: #cdd6f4;
            }
            QPushButton {
                background-color: #313244;
                color: #cdd6f4;
                border: 1px solid #45475a;
                border-radius: 4px;
                padding: 6px 14px;
            }
            QPushButton:hover {
                background-color: #45475a;
            }
        )");
    }
};

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("CHIP-8 Emulator");

    MainWindow window;
    window.show();

    if (argc > 1) {
        // If ROM path provided as CLI argument
        window.findChild<Chip8Canvas*>(); 
    }

    return app.exec();
}
