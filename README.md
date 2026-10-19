# Big Apple Sam

An After Dark–style night skyline you can zoom into. From far away it's just yellow window lights under a turning sky. Up close, every lit window is a small room with people living out their evenings.

Open `index.html` in a browser (or the GitHub Pages site). No build step and no dependencies.

## What's going on in there

- **One city per browser.** The skyline is generated once from a random seed and saved in `localStorage`, along with the clock and where you were looking. Come back tomorrow and it's the same city, and the clock has kept running while you were away.
- **Live clock.** By default the city runs at 1×, synced to your local time. Speed buttons go up to 600×; "Back to now" re-syncs, and "Skip to 7 PM" jumps to the evening.
- **A real sky.** You're looking south from about 42°N. Stars (including Orion, the Pleiades, Cygnus, Vega, Sirius and friends) rise and set with sidereal time, and the moon moves and shows the correct phase.
- **Day and night.** "Always night" is on by default. Untick it and the sun rises and sets on the real schedule for the date: twilight colors, a blue daytime sky, sunlit facades and glassy windows, with streetlights and headlights switching on at dusk.
- **About 6,000 residents.** Each has a job, a daily routine, two personality traits, a mood that shifts day to day, and a birthday. Many have a multi-week life arc: getting over a breakup, dating someone new, grieving a spouse, learning guitar, writing a novel, looking for work, training for a marathon, homesick, insomnia. Couples are newlyweds, steady, in a rough patch, or expecting (the baby arrives on a real date).
- **Scenes, not loops.** Every activity is a generated script of small beats: dinner burns and turns into a pizza order, someone cries at a movie, fights the TV antenna, dozes off on the sofa, gets up for water at 3 AM, sings into a hairbrush, hits a wrong note on guitar.
- **Shared stories.** Households have events: date nights with candles and slow dancing, arguments (sometimes one of them sleeps on the couch), curtains drawn for the night, parties with guests arriving in waves, birthdays with cake, bedtime stories, kids' nightmares, visitors, pizza delivery, game nights.
- **Get to know them.** Click a window for each resident's bio, today's mood, what they're doing and what's next, plus a "Lately" journal of the evening. "Follow" keeps the camera on that apartment.

## Controls

Scroll or pinch to zoom, drag to pan, click a window. Keys: `Space` pause, `1`–`4` speed, `P` peek into a random window, `F` follow, `Esc` close / zoom out, `+`/`-` zoom.
