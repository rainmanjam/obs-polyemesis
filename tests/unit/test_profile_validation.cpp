/*
 * OBS Polyemesis - Profile Validation Tests
 * Tests for profile name validation and creation rules
 */

#include <QtTest/QtTest>
#include <QString>
#include <QRegularExpression>

class TestProfileValidation : public QObject {
    Q_OBJECT

private:
    // Helper: Validate profile name rules
    bool isValidProfileName(const QString &name) {
        // Profile names must be:
        // - Non-empty
        // - 1-64 characters
        // - Alphanumeric, spaces, hyphens, underscores only
        // - No leading/trailing whitespace

        if (name.isEmpty() || name != name.trimmed()) {
            return false;
        }

        if (name.length() > 64) {
            return false;
        }

        QRegularExpression validChars("^[a-zA-Z0-9 _-]+$");
        return validChars.match(name).hasMatch();
    }

private slots:
    void initTestCase() {
        qDebug() << "Starting Profile Validation Tests";
    }

    void cleanupTestCase() {
        qDebug() << "Profile Validation Tests Complete";
    }

    void testValidProfileNames() {
        QVERIFY(isValidProfileName("My Profile"));
        QVERIFY(isValidProfileName("Profile_1"));
        QVERIFY(isValidProfileName("Test-Profile-2024"));
        QVERIFY(isValidProfileName("SimpleProfile"));
        QVERIFY(isValidProfileName("Multi Word Profile Name"));
    }

    void testInvalidProfileNames() {
        QVERIFY(!isValidProfileName(""));                    // Empty
        QVERIFY(!isValidProfileName(" LeadingSpace"));       // Leading space
        QVERIFY(!isValidProfileName("TrailingSpace "));      // Trailing space
        QVERIFY(!isValidProfileName("Invalid@Name"));        // Special char
        QVERIFY(!isValidProfileName("Bad!Profile"));         // Special char
        QVERIFY(!isValidProfileName("Profile/Slash"));       // Path separator
    }

    void testProfileNameLength() {
        QString validLength(64, 'a');
        QVERIFY(isValidProfileName(validLength));

        QString tooLong(65, 'a');
        QVERIFY(!isValidProfileName(tooLong));

        QString wayTooLong(1000, 'a');
        QVERIFY(!isValidProfileName(wayTooLong));
    }

    void testProfileNameEdgeCases() {
        QVERIFY(isValidProfileName("A"));                    // Single char
        QVERIFY(isValidProfileName("1"));                    // Number only
        QVERIFY(isValidProfileName("a-b_c 1"));              // Mixed valid chars
        QVERIFY(!isValidProfileName("   "));                 // Whitespace only
        QVERIFY(!isValidProfileName("\t\n"));                // Control chars
    }

    void testProfileNameUnicode() {
        QVERIFY(!isValidProfileName("Profile🎥"));           // Emoji
        QVERIFY(!isValidProfileName("Профиль"));             // Cyrillic
        QVERIFY(!isValidProfileName("配置文件"));              // Chinese
        // Note: Current implementation ASCII-only for safety
    }
};

QTEST_MAIN(TestProfileValidation)
#include "test_profile_validation.moc"
