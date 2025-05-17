#include <QApplication>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QMessageBox>
#include <QComboBox>
#include <QDateEdit>
#include <QHeaderView>
#include <QPrinter>
#include <QPrintDialog>
#include <QTextDocument>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlError>
#include <QGroupBox>

class SchoolFeesApp : public QWidget {
    Q_OBJECT
public:
    SchoolFeesApp(QWidget *parent = nullptr) : QWidget(parent) {
        // Database connection
        db = QSqlDatabase::addDatabase("QSQLITE");
        db.setDatabaseName("school_fees.db");
        if (!db.open()) {
            QMessageBox::critical(this, "Error", db.lastError().text());
            return;
        }

        // Create main tables
        QSqlQuery query;
        query.exec("CREATE TABLE IF NOT EXISTS fees ("
                   "receipt_no TEXT PRIMARY KEY, "
                   "student_name TEXT, "
                   "student_class TEXT,"
                   "year INTEGER,"
                   "amount REAL,"
                   "fee_type TEXT,"
                   "date TEXT"
                   ")");
        query.exec("CREATE TABLE IF NOT EXISTS fee_settings ("
                   "student_class TEXT, "
                   "year INTEGER, "
                   "fee_type TEXT, "
                   "amount REAL, "
                   "PRIMARY KEY(student_class, year, fee_type))");

        QVBoxLayout *mainLayout = new QVBoxLayout;

        // ************************************************
        // Payment entry fields
        QHBoxLayout *formLayout = new QHBoxLayout;
        receiptEdit = new QLineEdit;
        studentEdit = new QLineEdit;
        classCombo = new QComboBox;
        classCombo->addItems({"JSS1", "JSS2", "JSS3", "SS1", "SS2", "SS3"});
        yearEdit = new QLineEdit;
        amountEdit = new QLineEdit;
        feeTypeCombo = new QComboBox;
        feeTypeCombo->addItems({"Tuition", "Exam", "Library", "PTA", "Other"});
        dateEdit = new QDateEdit(QDate::currentDate());
        dateEdit->setCalendarPopup(true);
        formLayout->addWidget(new QLabel("Receipt No:"));
        formLayout->addWidget(receiptEdit);
        formLayout->addWidget(new QLabel("Name:"));
        formLayout->addWidget(studentEdit);
        formLayout->addWidget(new QLabel("Class:"));
        formLayout->addWidget(classCombo);
        formLayout->addWidget(new QLabel("Year:"));
        formLayout->addWidget(yearEdit);
        formLayout->addWidget(new QLabel("Amount:"));
        formLayout->addWidget(amountEdit);
        formLayout->addWidget(new QLabel("Fee Type:"));
        formLayout->addWidget(feeTypeCombo);
        formLayout->addWidget(new QLabel("Date:"));
        formLayout->addWidget(dateEdit);

        QPushButton *addButton = new QPushButton("Add/Update Payment");
        connect(addButton, &QPushButton::clicked, this, &SchoolFeesApp::addOrUpdatePayment);

        QPushButton *printButton = new QPushButton("Print Selected Receipt (PDF)");
        connect(printButton, &QPushButton::clicked, this, &SchoolFeesApp::printSelectedReceipt);

        // ************************************************
        // SEARCH
        QGroupBox *searchBox = new QGroupBox("Search Payments");
        QHBoxLayout *searchLayout = new QHBoxLayout;
        searchNameEdit = new QLineEdit;
        searchNameEdit->setPlaceholderText("Name");
        searchClassCombo = new QComboBox;
        searchClassCombo->addItem("All");
        searchClassCombo->addItems({"JSS1", "JSS2", "JSS3", "SS1", "SS2", "SS3"});
        searchYearEdit = new QLineEdit;
        searchYearEdit->setPlaceholderText("Year");
        searchFeeTypeCombo = new QComboBox;
        searchFeeTypeCombo->addItem("All");
        searchFeeTypeCombo->addItems({"Tuition", "Exam", "Library", "PTA", "Other"});
        QPushButton *searchButton = new QPushButton("Search");
        connect(searchButton, &QPushButton::clicked, this, &SchoolFeesApp::searchPayments);
        QPushButton *resetButton = new QPushButton("Reset");
        connect(resetButton, &QPushButton::clicked, this, &SchoolFeesApp::resetSearch);

        searchLayout->addWidget(new QLabel("Name:"));
        searchLayout->addWidget(searchNameEdit);
        searchLayout->addWidget(new QLabel("Class:"));
        searchLayout->addWidget(searchClassCombo);
        searchLayout->addWidget(new QLabel("Year:"));
        searchLayout->addWidget(searchYearEdit);
        searchLayout->addWidget(new QLabel("Fee Type:"));
        searchLayout->addWidget(searchFeeTypeCombo);
        searchLayout->addWidget(searchButton);
        searchLayout->addWidget(resetButton);
        searchBox->setLayout(searchLayout);

        // ************************************************
        // Table for payments
        feeTable = new QTableWidget;
        feeTable->setColumnCount(8);
        feeTable->setHorizontalHeaderLabels({"Receipt No", "Name", "Class", "Year", "Amount", "Fee Type", "Date", "Balance Due"});
        feeTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        feeTable->setSelectionBehavior(QAbstractItemView::SelectRows);
        feeTable->setEditTriggers(QAbstractItemView::NoEditTriggers);

        // ************************************************
        // Fee settings UI
        QHBoxLayout *settingsLayout = new QHBoxLayout;
        feeSettingsClassCombo = new QComboBox;
        feeSettingsClassCombo->addItems({"JSS1", "JSS2", "JSS3", "SS1", "SS2", "SS3"});
        feeSettingsYearEdit = new QLineEdit;
        feeSettingsTypeCombo = new QComboBox;
        feeSettingsTypeCombo->addItems({"Tuition", "Exam", "Library", "PTA", "Other"});
        feeSettingsAmountEdit = new QLineEdit;
        QPushButton *setFeeButton = new QPushButton("Set/Update Fee");
        connect(setFeeButton, &QPushButton::clicked, this, &SchoolFeesApp::setFee);

        settingsLayout->addWidget(new QLabel("Set Fee for Class:"));
        settingsLayout->addWidget(feeSettingsClassCombo);
        settingsLayout->addWidget(new QLabel("Year:"));
        settingsLayout->addWidget(feeSettingsYearEdit);
        settingsLayout->addWidget(new QLabel("Fee Type:"));
        settingsLayout->addWidget(feeSettingsTypeCombo);
        settingsLayout->addWidget(new QLabel("Amount:"));
        settingsLayout->addWidget(feeSettingsAmountEdit);
        settingsLayout->addWidget(setFeeButton);

        // ************************************************
        // BALANCES
        QGroupBox *balanceBox = new QGroupBox("Balances");
        QHBoxLayout *balanceLayout = new QHBoxLayout;
        perStudentBalanceLabel = new QLabel("Per Student Balance: ");
        overallBalanceLabel = new QLabel("Overall Balance: ");
        balanceLayout->addWidget(perStudentBalanceLabel);
        balanceLayout->addWidget(overallBalanceLabel);
        balanceBox->setLayout(balanceLayout);

        // ************************************************
        mainLayout->addLayout(formLayout);
        mainLayout->addWidget(addButton);
        mainLayout->addWidget(printButton);
        mainLayout->addWidget(searchBox);
        mainLayout->addWidget(new QLabel("All Payments:"));
        mainLayout->addWidget(feeTable);
        mainLayout->addWidget(balanceBox);
        mainLayout->addWidget(new QLabel("Standard Fee Settings (Admin):"));
        mainLayout->addLayout(settingsLayout);
        setLayout(mainLayout);

        // Load transactions and fee settings
        loadFees();
    }

private slots:
    void addOrUpdatePayment() {
        QString receiptNo = receiptEdit->text().trimmed();
        QString studentName = studentEdit->text().trimmed();
        QString studentClass = classCombo->currentText();
        QString yearStr = yearEdit->text().trimmed();
        QString amountStr = amountEdit->text().trimmed();
        QString feeType = feeTypeCombo->currentText();
        QString date = dateEdit->date().toString("yyyy-MM-dd");

        if (receiptNo.isEmpty() || studentName.isEmpty() || yearStr.isEmpty() || amountStr.isEmpty()) {
            QMessageBox::warning(this, "Input Error", "Please fill all required fields.");
            return;
        }
        bool ok;
        int year = yearStr.toInt(&ok);
        if (!ok || year < 2000 || year > 2100) {
            QMessageBox::warning(this, "Input Error", "Invalid year.");
            return;
        }
        double amount = amountStr.toDouble(&ok);
        if (!ok || amount <= 0) {
            QMessageBox::warning(this, "Input Error", "Invalid amount.");
            return;
        }

        QSqlQuery query;
        query.prepare("INSERT OR REPLACE INTO fees (receipt_no, student_name, student_class, year, amount, fee_type, date) "
                      "VALUES (?, ?, ?, ?, ?, ?, ?)");
        query.addBindValue(receiptNo);
        query.addBindValue(studentName);
        query.addBindValue(studentClass);
        query.addBindValue(year);
        query.addBindValue(amount);
        query.addBindValue(feeType);
        query.addBindValue(date);

        if (!query.exec()) {
            QMessageBox::critical(this, "Database Error", query.lastError().text());
            return;
        }

        loadFees();
        QMessageBox::information(this, "Success", "Payment recorded successfully.");
    }

    void setFee() {
        QString studentClass = feeSettingsClassCombo->currentText();
        QString yearStr = feeSettingsYearEdit->text().trimmed();
        QString feeType = feeSettingsTypeCombo->currentText();
        QString amountStr = feeSettingsAmountEdit->text().trimmed();

        if (yearStr.isEmpty() || amountStr.isEmpty()) {
            QMessageBox::warning(this, "Input Error", "Please fill all required fields for setting fees.");
            return;
        }
        bool ok;
        int year = yearStr.toInt(&ok);
        if (!ok || year < 2000 || year > 2100) {
            QMessageBox::warning(this, "Input Error", "Invalid year.");
            return;
        }
        double amount = amountStr.toDouble(&ok);
        if (!ok || amount < 0) {
            QMessageBox::warning(this, "Input Error", "Invalid amount.");
            return;
        }

        QSqlQuery query;
        query.prepare("INSERT OR REPLACE INTO fee_settings (student_class, year, fee_type, amount) VALUES (?, ?, ?, ?)");
        query.addBindValue(studentClass);
        query.addBindValue(year);
        query.addBindValue(feeType);
        query.addBindValue(amount);

        if (!query.exec()) {
            QMessageBox::critical(this, "Database Error", query.lastError().text());
            return;
        }

        QMessageBox::information(this, "Success", "Standard fee set/updated.");
        loadFees();
    }

    void loadFees(const QString &whereClause = "", const QList<QVariant> &binds = {}) {
        feeTable->setRowCount(0);
        QString queryStr = "SELECT * FROM fees";
        if (!whereClause.isEmpty()) queryStr += " WHERE " + whereClause;
        queryStr += " ORDER BY date DESC";

        QSqlQuery query;
        query.prepare(queryStr);
        for (const QVariant &v : binds) query.addBindValue(v);

        while (query.exec() && query.next()) {
            int row = feeTable->rowCount();
            feeTable->insertRow(row);
            for (int col = 0; col < 7; ++col)
                feeTable->setItem(row, col, new QTableWidgetItem(query.value(col).toString()));

            // Calculate balance due
            QString studentName = query.value(1).toString();
            QString studentClass = query.value(2).toString();
            int year = query.value(3).toInt();
            QString feeType = query.value(5).toString();
            double paid = getTotalPaid(studentName, studentClass, year, feeType);
            double due = getStandardFee(studentClass, year, feeType) - paid;
            if (due < 0) due = 0;
            feeTable->setItem(row, 7, new QTableWidgetItem(QString::number(due, 'f', 2)));
        }

        // Per student and overall balances
        updateBalances();
    }

    void printSelectedReceipt() {
        QList<QTableWidgetSelectionRange> ranges = feeTable->selectedRanges();
        if (ranges.isEmpty()) {
            QMessageBox::warning(this, "No Selection", "Please select a receipt row to print.");
            return;
        }
        int row = ranges.first().topRow();
        QString receiptNo = feeTable->item(row, 0)->text();
        QString studentName = feeTable->item(row, 1)->text();
        QString studentClass = feeTable->item(row, 2)->text();
        QString year = feeTable->item(row, 3)->text();
        QString amount = feeTable->item(row, 4)->text();
        QString feeType = feeTable->item(row, 5)->text();
        QString date = feeTable->item(row, 6)->text();
        QString balance = feeTable->item(row, 7)->text();

        QString html = QString(
            "<h2 style='text-align:center'>School Fees Payment Receipt</h2>"
            "<table width='100%' border='1' cellspacing='0' cellpadding='5'>"
            "<tr><td><b>Receipt No:</b></td><td>%1</td></tr>"
            "<tr><td><b>Name:</b></td><td>%2</td></tr>"
            "<tr><td><b>Class:</b></td><td>%3</td></tr>"
            "<tr><td><b>Year:</b></td><td>%4</td></tr>"
            "<tr><td><b>Fee Type:</b></td><td>%5</td></tr>"
            "<tr><td><b>Amount Paid:</b></td><td>%6</td></tr>"
            "<tr><td><b>Date:</b></td><td>%7</td></tr>"
            "<tr><td><b>Balance Due:</b></td><td>%8</td></tr>"
            "</table>"
            "<p style='margin-top:30px; text-align:center;'>Thank you for your payment!</p>"
        ).arg(receiptNo, studentName, studentClass, year, feeType, amount, date, balance);

        QPrinter printer;
        printer.setOutputFormat(QPrinter::PdfFormat);
        printer.setOutputFileName("receipt_" + receiptNo + ".pdf");

        QPrintDialog dialog(&printer, this);
        if (dialog.exec() == QDialog::Accepted) {
            QTextDocument doc;
            doc.setHtml(html);
            doc.print(&printer);
        }
    }

    // --- SEARCH logic ---
    void searchPayments() {
        QStringList clauses;
        QList<QVariant> binds;
        if (!searchNameEdit->text().trimmed().isEmpty()) {
            clauses << "student_name LIKE ?";
            binds << ("%" + searchNameEdit->text().trimmed() + "%");
        }
        if (searchClassCombo->currentText() != "All") {
            clauses << "student_class=?";
            binds << searchClassCombo->currentText();
        }
        if (!searchYearEdit->text().trimmed().isEmpty()) {
            clauses << "year=?";
            binds << searchYearEdit->text().trimmed().toInt();
        }
        if (searchFeeTypeCombo->currentText() != "All") {
            clauses << "fee_type=?";
            binds << searchFeeTypeCombo->currentText();
        }
        loadFees(clauses.join(" AND "), binds);
    }

    void resetSearch() {
        searchNameEdit->clear();
        searchClassCombo->setCurrentIndex(0);
        searchYearEdit->clear();
        searchFeeTypeCombo->setCurrentIndex(0);
        loadFees();
    }

    // --- BALANCE logic ---
    void updateBalances() {
        // Per student balance: for currently selected student in search, if any
        QString studentName = searchNameEdit->text().trimmed();
        QString display;
        if (!studentName.isEmpty()) {
            // Get all combinations of class, year, and fee type for this student
            QSqlQuery q("SELECT DISTINCT student_class, year, fee_type FROM fees WHERE student_name=?");
            q.addBindValue(studentName);
            double totalDue = 0, totalPaid = 0;
            while (q.exec() && q.next()) {
                QString sclass = q.value(0).toString();
                int syear = q.value(1).toInt();
                QString sftype = q.value(2).toString();
                double paid = getTotalPaid(studentName, sclass, syear, sftype);
                double stdfee = getStandardFee(sclass, syear, sftype);
                totalPaid += paid;
                totalDue += stdfee;
            }
            double balance = totalDue - totalPaid;
            if (balance < 0) balance = 0;
            display = QString("Per Student Balance for '%1': %2").arg(studentName).arg(balance, 0, 'f', 2);
        } else {
            display = "Per Student Balance: (Search by student name to view)";
        }
        perStudentBalanceLabel->setText(display);

        // Overall balance
        QSqlQuery qclass("SELECT DISTINCT student_class, year, fee_type FROM fee_settings");
        double allDue = 0, allPaid = 0;
        while (qclass.exec() && qclass.next()) {
            QString sclass = qclass.value(0).toString();
            int syear = qclass.value(1).toInt();
            QString sftype = qclass.value(2).toString();
            double stdfee = getStandardFee(sclass, syear, sftype);

            // Count all students who have at least one fee payment for this class/year/feetype
            QSqlQuery qstu;
            qstu.prepare("SELECT DISTINCT student_name FROM fees WHERE student_class=? AND year=? AND fee_type=?");
            qstu.addBindValue(sclass);
            qstu.addBindValue(syear);
            qstu.addBindValue(sftype);
            while (qstu.exec() && qstu.next()) {
                QString stu = qstu.value(0).toString();
                allDue += stdfee;
                allPaid += getTotalPaid(stu, sclass, syear, sftype);
            }
        }
        double overall = allDue - allPaid;
        if (overall < 0) overall = 0;
        overallBalanceLabel->setText(QString("Overall Balance: %1").arg(overall, 0, 'f', 2));
    }

private:
    double getTotalPaid(const QString &student, const QString &studentClass, int year, const QString &feeType) {
        QSqlQuery query;
        query.prepare("SELECT SUM(amount) FROM fees WHERE student_name=? AND student_class=? AND year=? AND fee_type=?");
        query.addBindValue(student);
        query.addBindValue(studentClass);
        query.addBindValue(year);
        query.addBindValue(feeType);
        if (query.exec() && query.next())
            return query.value(0).toDouble();
        return 0.0;
    }

    double getStandardFee(const QString &studentClass, int year, const QString &feeType) {
        QSqlQuery query;
        query.prepare("SELECT amount FROM fee_settings WHERE student_class=? AND year=? AND fee_type=?");
        query.addBindValue(studentClass);
        query.addBindValue(year);
        query.addBindValue(feeType);
        if (query.exec() && query.next())
            return query.value(0).toDouble();
        return 0.0;
    }

    // Payment entry
    QLineEdit *receiptEdit;
    QLineEdit *studentEdit;
    QComboBox *classCombo;
    QLineEdit *yearEdit;
    QLineEdit *amountEdit;
    QComboBox *feeTypeCombo;
    QDateEdit *dateEdit;
    QTableWidget *feeTable;
    QSqlDatabase db;

    // Fee settings
    QComboBox *feeSettingsClassCombo;
    QLineEdit *feeSettingsYearEdit;
    QComboBox *feeSettingsTypeCombo;
    QLineEdit *feeSettingsAmountEdit;

    // Search
    QLineEdit *searchNameEdit;
    QComboBox *searchClassCombo;
    QLineEdit *searchYearEdit;
    QComboBox *searchFeeTypeCombo;

    // Balances
    QLabel *perStudentBalanceLabel;
    QLabel *overallBalanceLabel;
};

#include <QtSql>
#include <QMetaType>
#include <QTableWidgetSelectionRange>

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    SchoolFeesApp window;
    window.setWindowTitle("School Fees Payment System");
    window.resize(1300, 600);
    window.show();
    return app.exec();
}

#include "school_fees_app.moc"