import schedule
import time
import threading#דאגה שפלאסק לא ייחסם
from flask import Flask, request, jsonify#ליצור פלאסק
import gspread#שליטה בגוגלשיטס
from oauth2client.service_account import ServiceAccountCredentials
import pytz#ניהול איזורי זמן
from datetime import datetime
import re  # ספרייה לזיהוי תבנית שם גיליון

app = Flask(__name__)

# 📌 התחברות ל-Google Sheets
scope = ["https://spreadsheets.google.com/feeds", "https://www.googleapis.com/auth/drive"]#מתן גישה אל גוגלשיטס
creds = ServiceAccountCredentials.from_json_keyfile_name("credentials.json", scope)
client = gspread.authorize(creds)

# 📌 פתיחת מסמך ה-Google Sheets
spreadsheet_id = "1edWMjBN64o2FiZJWqqJRIBMTTzHwykfZp73kgct3Lh0"
spreadsheet = client.open_by_key(spreadsheet_id)

# 📌 משתנה גלובלי לשמירת הגיליון הפעיל
active_sheet = None


def sheet_exists(sheet_name):
    """ בודק אם גיליון עם השם הנתון כבר קיים """
    try:
        spreadsheet.worksheet(sheet_name)
        return True
    except gspread.exceptions.WorksheetNotFound:
        return False


def get_latest_weekly_sheet():
    """ מחפש את הגיליון האחרון שתואם לתבנית 'Week X - 2025' """
    sheets = spreadsheet.worksheets()
    tz = pytz.timezone("Asia/Jerusalem")
    now = datetime.now(tz)
    current_week = now.isocalendar()[1]
    current_year = now.year

    pattern = re.compile(r"Week (\d+) - (\d{4})")  # זיהוי שמות גיליונות בפורמט הנכון

    for sheet in sheets:
        match = pattern.match(sheet.title)
        if match:
            week_number = int(match.group(1))
            year_number = int(match.group(2))

            if week_number == current_week and year_number == current_year:
                return sheet  # מצאנו את הגיליון המתאים
    return None


def set_column_widths(sheet):
    """ מכוון את רוחב העמודות בהתאם לתוכן """
    column_widths = {
        "D": 120,  # Date
        "E": 100,  # Time
        "F": 120,  # Day
        "G": 400,  # Temperature Outside
        "H": 300,  # Temperature Inside
        "I": 200  # Air Conditioner Temp
    }

    for col, width in column_widths.items():
        sheet.format(f"{col}:{col}", {"pixelSize": width})


def create_new_weekly_sheet():
    """ אם הגיליון הפעיל אינו של השבוע הנוכחי – יצירת גיליון חדש """
    global active_sheet

    tz = pytz.timezone("Asia/Jerusalem")
    now = datetime.now(tz)

    current_week = now.isocalendar()[1]
    current_year = now.year
    new_sheet_name = f"Week {current_week} - {current_year}"

    # אם קיים גיליון מתאים, נעבור אליו
    latest_sheet = get_latest_weekly_sheet()
    if latest_sheet:
        print(f"✅ נמצא גיליון קיים: {latest_sheet.title}")
        active_sheet = latest_sheet
        return

    # אם הגיליון אינו קיים – ניצור אותו
    print(f"🆕 יצירת גיליון חדש: {new_sheet_name}")
    new_sheet = spreadsheet.add_worksheet(title=new_sheet_name, rows="1000", cols="10")

    # 📌 הוספת כותרות
    expected_headers = ["Date ", "Time ", "Day ", "Temperature Outside (°C) ", "Temperature Inside (°C) ",
                        "Air Conditioner Temp (°C)"]
    new_sheet.update("D4:I4", [expected_headers])

    # 📌 עיצוב כותרת ראשית
    new_sheet.merge_cells("D3:I3")
    new_sheet.update("D3", [[f"ROLI WEATHER DATA - Week {current_week} ({current_year})"]])
    new_sheet.format("D3:I3", {
        "backgroundColor": {"red": 0.2, "green": 0.4, "blue": 0.8},
        "horizontalAlignment": "CENTER",
        "textFormat": {"bold": True, "fontSize": 16, "foregroundColor": {"red": 1, "green": 1, "blue": 1}}
    })

    # 📌 עיצוב כותרות עמודות (D4:I4)
    new_sheet.format("D4:I4", {
        "backgroundColor": {"red": 0.8, "green": 0.8, "blue": 0.8},  # רקע אפור בהיר
        "horizontalAlignment": "CENTER",
        "textFormat": {"bold": True, "fontSize": 12, "foregroundColor": {"red": 0, "green": 0, "blue": 0}}
    })

    # 📌 מרכוז הנתונים בשורות 5 ואילך
    new_sheet.format("D5:I1000", {
        "horizontalAlignment": "CENTER"
    })

    # 📌 התאמת רוחב העמודות
    set_column_widths(new_sheet)

    # 📌 עדכון הגיליון הפעיל לחדש
    active_sheet = new_sheet


# 📌 בדיקה יומית לוודא שהגיליון הנכון מופעל
schedule.every().day.at("00:05").do(create_new_weekly_sheet)


def run_scheduler():
    """ מריץ את הבדיקות ברקע כל דקה """
    while True:
        schedule.run_pending()
        time.sleep(60)


# 📌 הפעלת הבדיקה ברקע כדי לא לחסום את Flask
threading.Thread(target=run_scheduler, daemon=True).start()


@app.route('/upload_data', methods=['POST'])
def upload_data():
    global active_sheet

    try:
        data = request.get_json()
        if not data:
            return jsonify({"status": "error", "message": "No JSON received"}), 400

        arduino_time = data.get("time")
        temperature_outside = data.get("temperature_outside", "")
        temperature_inside = data.get("temperature_inside", "")
        fire_detected = data.get("fire_detected", "No").lower() == "yes"
        air_conditioner_temp = data.get("air_conditioner_temp", "")
        if fire_detected:
            air_conditioner_temp = f"🔥 FIRE! ({air_conditioner_temp}°C)" if air_conditioner_temp else "🔥 FIRE!"
        elif float(arduino_time[:2]) >= 22.00 or float(arduino_time[:2]) < 6.00:
            air_conditioner_temp = "🌙 NIGHT"
        else:
            air_conditioner_temp = f"🌞 DAY ({air_conditioner_temp}°C)" if air_conditioner_temp else "🌞 DAY"

        tz = pytz.timezone("Asia/Jerusalem")
        now = datetime.now(tz)
        date_today = now.strftime("%d/%m/%Y")
        day_of_week = now.strftime("%A")

        # 📌 וידוא שהגיליון הנכון מוגדר
        create_new_weekly_sheet()

        # 📌 הכנסת הנתונים לגיליון הפעיל
        current_row = len(active_sheet.col_values(4)) + 1

        new_row = [
            [date_today, arduino_time, day_of_week, temperature_outside, temperature_inside, air_conditioner_temp]]
        active_sheet.update(f"D{current_row}:I{current_row}", new_row)

        if fire_detected:
            color = {"red": 1, "green": 0, "blue": 0}  # 🔴 אדום (שריפה)
        elif float(arduino_time[:2]) >= 22.00 or float(arduino_time[:2]) < 6.00:
            color = {"red": 0.3, "green": 0.1, "blue": 0.5}  # 🟣 סגול כהה (לילה)
        else:
            color = {"red": 0.6, "green": 0.8, "blue": 1}  # 🔵 תכלת (נורמלי - המזגן פועל)

        active_sheet.format(f"D{current_row}:I{current_row}", {
            "backgroundColor": color,
            "textFormat": {"bold": True, "foregroundColor": {"red": 1, "green": 1, "blue": 1}}  # טקסט לבן
        })


        print(f"✅ נתונים נוספו בהצלחה לגיליון {active_sheet.title}: {new_row}")

        return jsonify({"status": "success", "message": "Data added successfully!"})

    except Exception as e:
        print(f"❌ שגיאה: {str(e)}")
        return jsonify({"status": "error", "message": str(e)}), 400


if __name__ == '__main__':
    print("🚀 Flask server is running at: http://0.0.0.0:8001")
    app.run(host='0.0.0.0', port=8001, debug=True)