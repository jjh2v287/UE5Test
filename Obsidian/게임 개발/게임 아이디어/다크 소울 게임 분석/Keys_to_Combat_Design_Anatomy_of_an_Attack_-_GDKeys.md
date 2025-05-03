Welcome to the next article on a series dedicated to Combat Design! It all started with one article, but it quickly became apparent that this topic is way too wide and complex to be able to tackle it with a single one.

This series is the logical succession of the article [Keys to Rational Enemy Design](http://gdkeys.com/keys-to-rational-enemy-design/), so make sure you check it out.

In this series, we will talk about all the components that a designer needs to consider to produce a **snappy, meaningful, rewarding, skill-based fight**:

- Pacing
- Enemy AI and decision trees
- Group AI
- Player’s abilities and metrics
- Player’s strategies
- Difficulty management and rationalization
- Hitboxes and Hurtboxes
- …
- And, of course, attacks, our topic of the day.

We are starting this series with this topic because attacks are the most core component of a fight. Dissecting what makes up an attack, both for the player and for the enemies the player is fighting will put in light the keys to a good fight: readability, fairness, skill, challenge, purpose…

We will talk about the different phases of an attack, about Windows of Opportunity (or **WoO** in short), about animation, about input lag… And, hopefully, you will have at the end of this article all the keys to challenge your own fight system and recognize the elements that make games like _Hollow Knight_ or _Nioh_ so good in that domain.

![](http://gdkeys.com/wp-content/uploads/2020/04/acattack-1024x559.jpg)

Assassin’s Creed II

But before we start, it’s important to understand the scope of what we will discuss: by combat, we mean **any real-time armed confrontation between the player and one or more enemies**. Our study will then be independent of said weapons, being fists, melee weapons, or ranged weapons. _Street Fighter, Punch-Out_, _For Honor, Assassin’s Creed, Hollow Knight, Cuphead, Mordhau, Dark Soul…_ Every one of these games confronts the player using globally the same design rules, and these are the ones we will break down today, starting with the enemy’s side of it.

## Enemy’s Attacks

In order to properly design an attack, we first need to understand what phases compose an attack. An enemy attack is on paper very straightforward, and always made of three distinct elements:

![](http://gdkeys.com/wp-content/uploads/2020/04/Group-95-1-1024x57.png)

The 3 phases of an attack

Think of it as in real life: should you want to throw a kick, you will first start preparing your body for it, bending your body backward, and arming your leg. This is the _anticipation_. Then comes the _attack_, the very small part that is about hitting the opponent. Finally, once the attack finishes, your body will have to go to its final resting position (that could be the original one): this is the _recovery_.

![](http://gdkeys.com/wp-content/uploads/2020/04/Group-100-1.png)

Visual representation of the 3 phases of an attack

Well, attacks in video games work in the exact same way:

![](http://gdkeys.com/wp-content/uploads/2020/04/Group-105.png)

Ryu’s Shoryuken

One reason is of course because our players, though playing in a fictional world, expect **real-life constraints and forces** to apply in it too: gravity, weight, anatomy, physiology… But we will come to that when talking about animation.

Another reason, and one that is particularly interesting for us game designers, is that these **three phases are critical for the gameplay, each one of them in its own way**. They are the base for players’ tactics to emerge and are carefully crafted to balance difficulty, challenge abilities, and drive playstyles.

Let’s develop each of them using the player experience as the common thread of this article.

> **Key #1**: Every single attack should be broken down into 3 phases: Anticipation, Attack, and Recovery.

### Anticipation

The anticipation of an attack is, in essence, a Sign for the player that a specific attack is coming (see. [Keys to Signs & Feedback](http://gdkeys.com/keys-signs-feedback/)). **It’s a warning**. It will drive the player to get away before the attack happens, block, dodge, or even push his luck and try hitting the enemy to interrupt the attack.

For the anticipation phase to fill its role properly, it must comply with a few rules:

- **The animation of each anticipation must be unique**: no other attack from this enemy can have the same anticipation (except if you want to pull a twist on your players, who am I to judge). And by unique, I mean **drastically different**. Subtleties are lost in the heat of a fight, players won’t ever see that an enemy slightly changed its stance before starting a different attack, and it will ultimately lead to frustration.
- **The animation of each anticipation must hint at its follow-up** **attack**: a player shouldn’t have to rely on memorization and trial and error to learn how to defeat an enemy. The animation, just like in real life, should follow logic (for example, bringing an ax behind your body before splitting a log in two). Record and study your players’ first encounter with your enemies. Even if they get hit by an attack, they should have seen it coming and started reacting accordingly.

> **Key #2**: an efficient Anticipation translates in a unique warning animation, telegraphing the incoming attack.

_Cuphead_ is a beautiful reference to study. Their over-exaggerated animation style and the cartoon codes of the 1930s allowed them to really create the most legible combat animations I’ve seen in a game.

I mean… Look at that anticipation!

![](http://gdkeys.com/wp-content/uploads/2020/04/cuphead.gif)

Cuphead

Now that we talked about **animation unicity and continuity** there is still one big point to discuss relative to the anticipation: **timing**. How long should this anticipation last? Or, in development terms: **how many frames must the anticipation have?** Well, unfortunately, there is no clear rule for that as it depends on two factors: **what action do you expect the player to do (and how long does it take to do it)? And what is the extra margin you want to give them?** Remember that acknowledging an incoming danger and reacting to it isn’t immediate for a human being (unfortunately).

As a rule of thumb, **consider that the reaction time of a player will be 0.25 seconds** (so around 8 frames in a 30fps game, and 15 frames in a 60fps one). Now let’s say you want the player to block a specific attack, and that the block will be efficient at the end of its animation, 10 frames after the press of the block button by the player. The _minimum_ anticipation time will be 10+8= 18 frames! Of course, other variables could affect this number: the extra frames the weapon will need to reach the players based on their distance from the enemy, the input lag, etc. But you get the idea.

**The difficulty of dodging/parrying/interrupting/blocking the attack** will then **crystallize in a single metric**: how much of an **extra buffer** do you give your players on top of this minimum anticipation time? This part will be a constant fine-tuning between players and developers. Playtests are key.

> **Key #3**: Anticipation time = (player’s reaction time) + (expected player’s ability trigger time) + (difficulty buffer).

### Attack

The attack part is the most obvious for us to imagine as it mimics perfectly real-life scenarios. The enemy will try to use its weapon (a fist, a sword, a gun) to hit a vulnerable part of our player: usually its body. Easy enough.

That being said, there are a lot of subtleties that will make an attack _feel_ good. What we are looking for here is a **sharp, snappy feeling** and not a sloppy unpredictable one.

When designing an attack, you want it to be:

- Instant
- Unambiguous
- With a clear intent
- Precise
- Driving a specific player’s reaction

It seems straight-forward enough and yet plenty of games miss the spot with these directions. Having an instant attack means that **your animation must not be longer than a few frames from the end of the anticipation to the start of the recovery**. This means you will probably have to **bend the laws of physics** slightly to exaggerate the movement. Of course, we can find plenty of examples of extremely slow, pressuring attacks but these do not change the goal we are trying to achieve: **we want the player to be able to tell _exactly_ when the attack will land**. Any blurriness at that moment will result in a sloppy, frustrating attack.

Driving a specific player’s reaction and being unambiguous means that you want your **attack to be pretty stereotypical and its metrics to fit the player’s abilities**. For example, you do not want a wavy, diagonal attack with varying speed. Instead, you will prefer a clean horizontal swipe with a constant speed.

Look at that Yokai’s double attack from _Nioh:_

![](http://gdkeys.com/wp-content/uploads/2020/04/HarshFrailDrake-size_restricted.gif)

Nioh combat

Remember what I was saying about bending the laws of physics? The anticipation is slow and involves the entire Yokai’s body, showcasing the insane weight (and deadly power) of that ax. Considering the struggle to lift this weapon, there is no way the demon could swing it that fast. But at that specific moment, anatomy and realism do not matter. What matters is that his swing is instant and perfectly horizontal, allowing the player to precisely time his dodge under it.

The follow-up attack follows the same principles: perfectly vertical and happening in a blink. You will notice that this attack cannot hit a player that dodged the first hit (the Yokai doesn’t reorient mid-attack). It is used either as a Window of Opportunity (WoO) for the players to land their own counter-attack or a **double punishment should a player get hit by the first blow**.

> **Key #4**: an attack is instant, constant, and with a very small amount of varying parameters.

Side note: look at the anticipation. It is unique, recognizable, and without subtleties. The downside I see is how close this ax rotation is from looking like an attack. _Nioh_ actually deals with these issues (as they are common among their enemies) by **adding a VFX to the weapon during the attack phase** (i.e. the dangerous part). **Weapon trails** are very efficient Signs and support the learning and understanding of the attacks and patterns of enemies. Plenty of games use this trick, you could consider it too!

> **Key #5**: Adding a weapon trail VFX to all attacks, following the weapon and its hitbox, is a clean way to highlight its deadly part (and the VFX persistence will underline the attack direction).

### Recovery

Finally, once an attack has been performed, comes the recovery. And let’s make it clear right away, **the goal of this phase is extremely simple: give the player a clear Window of Opportunity**. A window to hit back, to heal, to run away, to reload, to plan a change of tactic… This recovery is a small breather given to the player in the fight. And compared to the two previous phases, there are no rules to respect here.

**The form can take whatever shape you want**: an enemy holding its final attack position for a bit longer (very easy to implement, looks badass as hell), a foe dislodging its weapon stuck in the ground after a landing attack, or an exhausted animation after a particularly brutal attack… The possibilities are endless!

**The timing is also completely open**: from a single frame for chaining move without openings to a full 10s standing-up animation!

![](http://gdkeys.com/wp-content/uploads/2020/04/bloodborne.gif)

Bloodborne

The enemies in _Bloodborne_ often drag back their weapon into position after an attack, leaving plenty of time for a player for a counter-attack (which is balanced compared to how deadly each attack is), as highlighted by the enemy at the end of that gif.

> **Key #6**: A recovery is driven by its timing. This timing is decided by the length of the WoO you want to give to your players.

## Enemy Case Study

Now that we have broken down each phase separately, let’s see how all of them can combine in full-fledged attacks, how a set of attacks is balanced, and how it drives a fight to be unique and memorable.

We are going to study one of the boss fights from _Hollow Knight_: **the first fight against Hornet**. Hornet is a particularly feared enemy at the start of the game (leading to pages of rage on forums), but we are going to see that she is actually **particularly well-designed**. My understanding is that, compared to all the previous bosses the player had to fight, Hornet is the first boss to really adapt to the player: she is breaking patterns, having attacks triggering on the player’s proximity, targeting the player’s location precisely, etc. This is a fresh take on the gameplay, one that unsettles players much.

Hollow Knight Hornet boss fight

For the sake of that example, we will need 3 metrics:

- **We will consider the game is running at 30fps** (the game actually runs at a steady 60FPS).
- **Player’s attacks are pretty much instantaneous**: a few frames long and triggering without anticipation and minimal recovery.
- **Healing** is an ability that requires **45 frames uninterrupted** (1.5s) while locking the player on the spot, vulnerable, for its entire duration.

Hornet has only 4 attacks:

- **Dash**: on the ground, dashing towards the player, directional.
- **Throw**: on the ground, launching her weapon through most of the arena, and retracting it afterward (two attack moments), directional.
- **Lasso**: on the ground or in-air, triggered as a protective measure, pushes the player away, 360 degrees.
- **Dive**: in-air, jump, and dive towards the exact player location (calculated at the start of the dive, uncorrected mid-attack)

Here is the **break-down of each attack, phase by phase, with timings**:

![](http://gdkeys.com/wp-content/uploads/2020/04/Group-94-1.png)

Hornet various attacks breakdown

And this breakdown tells us much about how great the design of this enemy is! Let’s start by looking at the different phases of these attacks:

![](http://gdkeys.com/wp-content/uploads/2020/04/Group-112-1024x123.png)

Various anticipation shapes

The anticipation of **each attack has a distinctive, unique animation**. Even on a small character with little features, Team Cherry managed to create highly readable attacks. A very powerful approach, as highlighted by this picture, is to combine this unique animation with a distinctive body shape. This is a very powerful tool in attack memorization.

> **Key #7**: The global shape of an enemy in an animation is a strong vector of unicity while staying subtle and unintrusive.

![](http://gdkeys.com/wp-content/uploads/2020/04/Group-110.png)

A unique VFX per attack keep them very distinct

Attacks, themselves very distinct, are all **highlighted by a unique VFX** (the dash attack VFX being a simple motion blur). Not only it is a clear sign for the player (like the one we’ve seen in the Nioh example), but it also helps differentiate each one of them.

![](http://gdkeys.com/wp-content/uploads/2020/04/Group-111-1024x128.png)

Identical recoveries, going back to Idle pose

Finally, the recoveries are all identical and are basically the resting pose of Hornet. This allows Team Cherry to **cancel a recovery at any time** (for example if considered that this specific recovery WoO has been exploited enough by the player) **while making sure that the animation transition stays perfect**. But this is a topic for another article.

> **Key #8**: Should you need to cancel an enemy animation, focus on the transition frames and consider it to be the idle pose of this enemy.

But there is another extremely interesting aspect to that fight breakdown, one that will be **key in the definition of the enemy’s difficulty and driving the unicity of the gameplay: timings**.

### Hornet’s Attacks Timing

The timings of each phase of each attack play a critical role: they **define the gameplay**. They tell players what they can, and can’t do: when to attack, when to back off, when to push their luck, when to block… And each attack needs its own design and specificities.

In that light, Hornet’s attacks are a very clean example:

![](http://gdkeys.com/wp-content/uploads/2020/04/Group-113.png)

Dash attack breakdown

- **Dash**
  - _Anticipation_: 15f (0.5s), the shortest of all anticipations. It creates a very nervous attack really **challenging the player’s reflexes** (the difficulty buffer is minimal). This is just enough time for the player to notice the animation and jump to safety.
  - _Attack_: 10f, short-range, very fast, in the continuity of the anticipation.
  - _Recovery_: 10f. This is interesting: it is way too short for players to counter-attacks in most situations, **this timing actively denying a WoO**. But this attack having constant displacement metrics, a skilled player could arrange to be almost in contact when the attack finishes, exploiting this extremely small WoO to land a hit.

![](http://gdkeys.com/wp-content/uploads/2020/04/Group-114.png)

Throw attack breakdown

- **Throw**
  - _Anticipation_: 20f. More relaxed, leaving the player time to anticipate this dangerous attack.
  - _Attack_: 20f+15f. Played two times (a back and forth of the weapon), with a range covering almost the entirety of the arena, it invites the player to jump inside the attack and exploit the full second of WoO to land up to 2 hits to Hornet.
  - _Recovery_: 10f. As previously discussed, this timing isn’t enough for the player to take advantage of it, effectively emphasizing the previous attack phase WoO.

![](http://gdkeys.com/wp-content/uploads/2020/04/Group-115.png)

Lasso attack breakdown

- **Lasso**
  - _Anticipation_: 20f. This attack triggers in response to the player’s proximity and will deny the immediate vicinity of Hornet, the extra frames allow the player to back off to safety.
  - _Attack:_ 30f. This is also very interesting. This attack could have happened in a blink (a pushback), and there is no need to make it last for that long. What is really happening here, is that **this attack creates the biggest WoO of the fight**. Not to hit Hornet (as she is protected), but instead, **to heal**: this is the only clean and safe moment in the fight for the player to do it, and the designers at Team Cherry made sure of that.
  - _Recovery_: again, very long (almost 1 second) to let the player finish healing, or get in contact and counter-attack.

![](http://gdkeys.com/wp-content/uploads/2020/04/Group-116.png)

Dive attack breakdown

- **Dive**
  - _Anticipation_: very long anticipation to let the player prepare for the very dangerous attack to come: one that targets the player’s exact position.
  - _Attack_: average length and difficulty
  - _Recovery_: a small WoO, but usable for the players as Hornets will stop next to them if they dodge the hit.

> **Key #9:** The timings of an attack, in its 3 phase, will define the gameplay and drive the player’s opportunities and tactics.

This analysis, as long as it took to write it, only looked at the attacks of Hornet. Pacing, AI, and Arena are all different topics that would require the same careful study. **Designing an enemy is a long and precise process** and an unavoidable one in a serious combat-oriented game. Many games put most of their energy on the player side of the fight, and this is a mistake.

I really invite you to check every single enemy and boss of _Hollow Knight_. They are all wonderfully designed and an infinite source of inspiration.

### Animation

Covering animation in combat design is obviously a topic that deserves its own article. In the meantime, I invite you to check this video to close our _Hollow Knight_ case study:

The animation of Hollow Knight

As a reminder, here is what we discussed already:

- _Anticipation_: the animation must be **unique in shape and pose** and should **hint at the incoming** **attack**.
- _Attack_: the animation must be **instant** (even if it needs to bend the rules of physics), with **ca lear hit location** and **consistent direction**.
- _Recovery_: the player WoO. The animation must be **clearly Signed** and could **end up in an idle pose** for an easy transition.

A game designer paired with an animator is a key combination in combat design. Animators will work with muscles, gravity, weight, anatomy… A game designer will work with timings, WoO, intended behavior, tactics, pattern recognition… **These domains will clash constantly** and the sweet spot will be found by a **constant back and forth and close collaboration between these two**.

> **Key #10**: A state-of-the-art fight will be found at the intersection of Game Design and Animation. These domains should feed each other.

But how about the other side of the attack topic? The player side?

## Player’s Attacks

A player’s attack construction is surprisingly close to an enemy’s attack one. It has but **a big addition: the _Trigger_ Part**.

![](http://gdkeys.com/wp-content/uploads/2020/04/Group-96-1-1024x51.png)

Player-side, we add an extra step: the trigger

Compared to an enemy that instantly triggers an attack when deciding to, the player will have to go through a **time-heavy process**: the trigger of the attack. This process involves quite some steps:

- The mental process of deciding to launch a specific attack and sending the information to the hand.
- The time it takes to physically press a button (triggers and dead zones being the worst).
- The latency between the press of this button and the actual registration by the game (electric transfer, etc.).
- **The time it takes for the game to act on it and actually trigger the attack.**

The first 3 elements are unavoidable, and pretty much outside of our control (most are even player’s setup dependent). But the last item is actually one to be very careful about and this article is a good opportunity for us to dig a bit into it. This is what we commonly refer to when we talk about **input lag**.

### Input Lag

The input lag can be generated by plenty of different sources, from the hardware itself, the network connectivity, the FPS count, the gameplay algorithms, or even some animation dead frames.

![](http://gdkeys.com/wp-content/uploads/2020/04/input-lag-skyrim.gif)

Example of Input Lag in Skyrim

Unfortunately, however good you are, you won’t ever avoid it. This issue may lead to players’ discomfort and frustration and will directly impact the feeling of sharpness of your gameplay and if your game feels _Responsive._

_Super Meat Boy_, _Hollow Knight,_ or _CoD: Modern Warfare 2_, played in the best conditions, all have an **input lag inferior to 100ms**. This is what you should aim for. _GTA4_ and _Battlefield 3_ both have an average of 160ms, which is perceivable and directly affects gameplay. In the case of _GTA_, it may not be critical as the game pillars do not revolve around a surgical combat design, but this is a reason to me why _CoD_ has always felt tighter than _Battlefield_ (though _BF4_ went down to an average of 95ms and came back in the FPS race).

Try and measure the input lag on each of your character’s actions (jump, dash, hit, block, etc.). If you have issues, before digging into the engine and graphic algorithms, try checking for those classic problems:

- This may not be input lag, but simply some dead frames **at the start of your animation**.
- You have a **drop in FPS**, a very common issue for reactivity (particularly if an attack is heavy on VFX).
- Your **gameplay algorithms are “waiting”** for a few frames.

Relative to this last point, I worked on a game that was checking for 10 frames if a player input was a tap or a hold before processing the result… This was 10 lost frames on each action and an easy win while bringing down the input lag.

> **Key #11**: time all your character’s actions, and make sure the critical ones (precise combat, pixel-perfect platforming, quick-time events, etc.) all fall under 100ms.

### Anticipation & Recovery

Now, let’s go quickly through the Anticipation and Recovery of a player’s attack. I grouped them together because, compared to what we have seen before on our enemies, these 2 phases are identical in purpose and a lot more straightforward: they are here to **provide risk-reward situations** and **create diversity in the gameplay**.

Games like _Dark Soul_ are famous for their combat approach revolving around the player’s commitment: **once the player triggers an attack, it can’t be canceled**. Trying to land an attack means that the player will be **extremely vulnerable during its anticipation and recovery**. It then becomes a trade-off: promising greater damages in exchange for higher risks of being hit and interrupted.

_Nioh_ not only plays with that logic with its weapons but also with a beautifully designed system of stances:

![](http://gdkeys.com/wp-content/uploads/2020/04/for-gif.gif)

Nioh stances system

> **Key #12:** Precisely decide if the player’s actions can be canceled and, if the answer is yes, during which time frame.

Feel free to experiment with these 2 phases: _Hollow Knight_, for example, has a single frame of anticipation and an instant recovery on its basic attack, driving less punishing and more nervous combat (while still not being cancelable).

### Attack

Finally, the attack part.  
This one follows the same construction rules as the enemy’s:

- Instant
- Unambiguous
- With a clear intent
- Precise
- **Answering a specific player’s need**

A sharp and crisp combat system is, in the end, but a constant back and forth between the player and the enemies, defined by metrics, timing, and attack specificities. Those will play a crucial role in driving what players can and can’t do at every moment and letting them build strategies and develop skills to play around them.

Our next articles will push this even further, by looking not only at how attacks define a fight, but how **pacing, AI,** and **group AI** organize these attacks in a deadly, rewarding choreography.

---

Did you enjoy this article? If so, please consider supporting GDKeys on Patreon. It helps keep this place free of ads and paywalls, ensures we can keep delivering high-quality content, and **gives you access to our private Discord** where designers worldwide gather to discuss design and help each other on their projects.

[![](http://gdkeys.com/wp-content/uploads/2020/03/Group-57-1024x121.png)](https://www.patreon.com/GDKeys)
