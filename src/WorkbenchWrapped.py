import ssl
# Tell your computer's internet request library to ignore unconfigured local SSL issues
ssl._create_default_https_context = ssl._create_unverified_context

import pandas as pd
from google import genai
from google.genai import types

# ==========================================
# 1. SET UP YOUR GEMINI KEY
# ==========================================
client = genai.Client(api_key="INSERT")

# ==========================================
# 2. FETCH LIVE DATA STRAIGHT FROM GOOGLE SHEETS
# ==========================================
print("🌐 Accessing live Google Sheet spreadsheet data...")

# PASTE YOUR SPREADSHEET ID HERE
# It's the long string between /d/ and /edit in your Google Sheet URL
SPREADSHEET_ID = "INSERT"

# This special URL tells Google to export your public sheet dynamically as a CSV download
live_sheet_url = f"https://docs.google.com/spreadsheets/d/{SPREADSHEET_ID}/export?format=csv"

# Load the live cloud dataset directly into Pandas
df = pd.read_csv(live_sheet_url)

if df.empty or len(df) < 2:
    print("⚠️ Your Google Sheet is empty or lacks enough usage logs yet! Go tap some trays first.")
    exit()

# Convert timestamp string column into live datetime trackers
df['timestamp'] = pd.to_datetime(df['timestamp'])

# Filter data matrix parameters: We only calculate active duration metrics on "stowed" actions
stow_actions = df[df['action'] == 'stowed']

total_seconds = stow_actions['duration_out_seconds'].sum()
total_hours = round(total_seconds / 3600, 1)

tray_totals = stow_actions.groupby('tray')['duration_out_seconds'].sum()
favorite_tray = tray_totals.idxmax()
favorite_tray_hours = round(tray_totals.max() / 3600, 1)

longest_session_hours = round(stow_actions['duration_out_seconds'].max() / 3600, 1)

df['hour'] = df['timestamp'].dt.hour
late_night_sessions = df[df['hour'] >= 22].shape[0]

# ==========================================
# 3. GENERATE GEMINI REPORT NARRATIVE
# ==========================================
print("🔮 Handing metrics to Gemini...")

prompt_content = f"""
You are a witty, enthusiastic AI assistant creating a "Spotify Wrapped" style report for a hardware maker's workbench project tracking station. 

Review these raw workbench stats from the past week:
- Total time spent creating: {total_hours} hours.
- Favorite Tray: The {favorite_tray} Tray ({favorite_tray_hours} hours total).
- Longest single continuous session: {longest_session_hours} hours.
- Late-Night Sessions: {late_night_sessions} projects started after 10:00 PM.

Write a fun, punchy, card-by-card report summarizing their week. 
Include:
1. A unique, creative "Maker Archetype" title based on their habits.
2. A breakdown of their favorite tray with a humorous observation.
3. A playful remark celebrating or teasing their late-night hyperfocus habits.

Keep the tone energetic, appreciative of engineering/building, and deeply engaging!
Use fun emojis at the start of cards to make it visually scannable.
"""

response = client.models.generate_content(
    model='gemini-2.5-flash',
    contents=prompt_content,
    config=types.GenerateContentConfig(
        system_instruction="You are an expert copywriter specializing in gamified productivity summaries.",
        temperature=0.7
    )
)

print("==========================================")
print("🎉 YOUR LIVE WEEKLY MAKER WRAPPED IS READY! 🎉")
print("==========================================")
print(response.text)