# Big Apple Sam

An After Dark–style night skyline you can zoom into. From far away it's just yellow window lights under a turning sky. Up close, every lit window is a small room with people living out their evenings.

Open `index.html` in a browser (or the GitHub Pages site). No build step and no dependencies.

## What's going on in there

- **One city per browser.** The skyline is generated once from a random seed and saved in `localStorage`, along with the clock and where you were looking. Come back tomorrow and it's the same city, and the clock has kept running while you were away.
- **Live clock.** By default the city runs at 1×, synced to your local time. Speeds go 60×, 600×, 6000× (a day in about four minutes) and 1 day/s (a year in about six minutes). "Back to now" re-syncs, and "Skip to 7 PM" jumps to the evening.
- **Years go by.** Fast-forward and the city ages. Every apartment has its own deterministic history: people grow older and go gray, kids grow up and leave at 18, students graduate, workers retire, couples move in together, hit rough patches, split up or have babies (who grow into kids with their own beds), pets come and go, and people eventually die. Households move out, the window goes dark with a FOR RENT sign, and new tenants arrive with their own furniture. Each apartment's window shows an "Over the years" timeline.
- **A real sky.** You're looking south from about 42°N. Stars (including Orion, the Pleiades, Cygnus, Vega, Sirius and friends) rise and set with sidereal time, and the moon moves and shows the correct phase.
- **Day and night.** The sun rises and sets on the real schedule for the date (tick "Always night" to keep it dark): twilight colors, a blue daytime sky, sunlit facades and glassy windows, with streetlights and headlights switching on at dusk.
- **About 6,000 residents.** Each has a job, a daily routine, two personality traits, a mood that shifts day to day, and a birthday. Many have a multi-week life arc: getting over a breakup, dating someone new, grieving a spouse, learning guitar, writing a novel, looking for work, training for a marathon, homesick, insomnia. Couples are newlyweds, steady, in a rough patch, or expecting (the baby arrives on a real date).
- **Scenes, not loops.** Every activity is a generated script of small beats: dinner burns and turns into a pizza order, someone cries at a movie, fights the TV antenna, dozes off on the sofa, gets up for water at 3 AM, sings into a hairbrush, hits a wrong note on guitar.
- **Shared stories.** Households have events: date nights with candles and slow dancing, arguments (sometimes one of them sleeps on the couch), curtains drawn for the night, parties with guests arriving in waves, birthdays with cake, bedtime stories, kids' nightmares, visitors, pizza delivery, game nights.
- **Get to know them.** Click a window for each resident's bio, today's mood, what they're doing and what's next, plus a "Lately" journal of the evening. "Follow" keeps the camera on that apartment.

## Controls

Scroll or pinch to zoom, drag to pan, click a window. Keys: `Space` pause, `1`–`5` speed, `P` peek into a random window, `S` screensaver (autopilot), `F` follow, `Esc` close / zoom out, `+`/`-` zoom.

**Screensaver button.** Hands the camera to an autopilot that drifts along the skyline, picks a lively apartment (a party, a date night, someone up at 3 AM), zooms in and lingers with a caption, sometimes hops next door, then pulls back. Move the mouse or press a key to take over again.

**City code.** The controls window shows a code like `bod5nr-g09`. Open the page with `#city-bod5nr-g09` on the end of the address, or paste the code into the screen saver's options, to watch the same city somewhere else.

## macOS screen saver

`saver/` wraps the page in a native `Big Apple Sam.saver` that runs the autopilot full screen, at 30 fps, on every display (each display follows a different part of the city). GitHub Actions builds it on every push and attaches it to the [latest release](../../releases/tag/latest).

1. Download `Big-Apple-Sam-saver.zip` from the latest release and unzip it.
2. Double-click `Big Apple Sam.saver`. The build isn't signed with an Apple Developer ID, so macOS will refuse the first time. Open **System Settings → Privacy & Security**, scroll down, and click **Open Anyway** next to the Big Apple Sam message. (Or in Terminal: `xattr -dr com.apple.quarantine ~/Downloads/"Big Apple Sam.saver"`, then double-click again.)
3. Choose to install it for this user. It appears under **System Settings → Screen Saver**.
4. Click **Options…** to paste a city code, pick the clock speed (live by default), and turn "Always night" or the captions on and off.

To build it yourself on a Mac: `brew install xcodegen`, then in `saver/` run `xcodegen generate` and open `BigAppleSam.xcodeproj`.
