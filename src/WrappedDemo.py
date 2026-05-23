import pandas as pd
from google import genai
from google.genai import types
import os

# ==========================================
# 1. SET UP YOUR GEMINI API KEY
# ==========================================
# Easiest way to start: Replace "your-gemini-key-here" with your actual key.
# You can generate a free developer key at: aistudio.google.com
client = genai.Client(api_key="")

# ==========================================
# 2. THE MATH: CALCULATE WORKSPACE STATS
# ==========================================
print("📊 Analyzing your workspace history...")
#f = open("historydemo.csv", "x")
# Load our CSV data file using Pandas
df = pd.read_csv('historydemo.csv')

# Convert the timestamp text column into actual dates/times Python understands
df['timestamp'] = pd.to_datetime(df['timestamp'])

# Calculate total seconds spent working, then convert to hours
total_seconds = df['duration_out_seconds'].sum()
total_hours = round(total_seconds / 3600, 1)

# Figure out which tray was used the most total time
tray_totals = df.groupby('tray')['duration_out_seconds'].sum()
favorite_tray = tray_totals.idxmax()
favorite_tray_hours = round(tray_totals.max() / 3600, 1)

# Find your longest single project session
longest_session_hours = round(df['duration_out_seconds'].max() / 3600, 1)

# Extract the hours (0-23) to see what time of day you work most
df['hour'] = df['timestamp'].dt.hour
late_night_sessions = df[df['hour'] >= 22].shape[0] # Count sessions starting after 10 PM

# ==========================================
# 3. THE MAGIC: PASS STATS TO GEMINI
# ==========================================
print("🔮 Handing data to Gemini to generate your Wrapped Report...\n")

# Build the prompt dynamically using our math calculations
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

# Send the data packet to the Gemini 2.5 Flash model
response = client.models.generate_content(
    model='gemini-2.5-flash',
    contents=prompt_content,
    config=types.GenerateContentConfig(
        system_instruction="You are an expert copywriter specializing in gamified productivity summaries.",
        temperature=0.7
    )
)

# Print out the results!
print("==========================================")
print("🎉 YOUR WEEKLY MAKER WRAPPED IS READY! 🎉")
print("==========================================")
print(response.text)