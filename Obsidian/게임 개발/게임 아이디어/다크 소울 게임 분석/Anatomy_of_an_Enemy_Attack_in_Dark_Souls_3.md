In my last [article](https://www.gamedeveloper.com/game-platforms/why-do-we-play-dark-souls-), I tried to explain why we play Dark Souls from the perspective of Competence motivation and analyzed how they create a Competence experience from Gameplay Challenge Balance, Gameplay Clarity, and Meaningful Feedback. This time we dig deeper by breaking down an enemy attack animation and have a detailed look at how they create fairness and clarity in Dark Souls 3's combat. On the other hand, this article also attempts to build a framework from the gameplay perspective to analyze the action games' attack animation and the design intent behind it.

Phases of the Dark Souls 3 attack animation

Generally, the attack animation has three phases: Anticipation, Attack, and Recovery.

But in Dark Souls 3, according to the Demon Souls' attack animation break down in Eric Jacobus's Blog and its emphasis on poses that evolved from Japanese traditional Kabuki, I divide it into 5 phases in total to illustrate my inference.

![](https://eu-images.contentstack.com/v3/assets/blt740a130ae3c5d529/bltdd075144d7cf3b45/650f1238092b4344145b3fbc/52085782257_edb67a5066_z.jpg/?width=700&auto=webp&quality=80&disable=upscale)

(Figure 1) 5 phases of Black Knight's swing attack

Opening Pose: The process from Idle pose to Opening Pose. It's telegraphing an attack, notifying the player to prepare for a response.

Attack Signal: This is a transition from Opening Pose to Attack, and the hitbox is not yet active. Players are required to pay attention to the moving arc of the great sword (Figure 1) and be ready to press the countermeasure.

Attack: Hitbox takes effect in this phase.

End Pose: End Pose represents the end of an attack. It could either be followed by Return or canceled by another enemy move.

Return: The process from End Pose to Idle Pose, usually the phase that gives players the Window of Opportunity (or WoO in short).

When such an attack is coming, players are asked to react. In Dark Souls 3, although the player has many choices to react, not every player can do it successfully, especially when facing tough enemies, but they keep trying and learning their moves until they beat them. To achieve such a challenge loop in a high-intensity game makes it very demanding for combat fairness and clarity. Before we can figure out how Dark Souls 3 achieve this, we first need to analyze what the player is going through in response to an attack.

The Interaction Model

Here we use the interactive model from Steve Swink's book "Game Feel".

![](https://eu-images.contentstack.com/v3/assets/blt740a130ae3c5d529/blt38239f1749f15d3d/650f0cf1a4192f482235698b/52100913737_aff413daaf_z.jpg/?width=700&auto=webp&quality=80&disable=upscale)

(Figure 2)Steve Swink - Interactivity in detail (Simplified version)

The whole process has two halves, on the player side, the player perceives the state of the game, thinks about how to react, and finally passes the impulse to his muscles to react. On the computer side, the computer receives the input signal, and then it has to process and output the result within a limited time to ensure an immediate response.

The measurements below are from the "Model Human Processor" (Card, Moran, and Newell, 1986) after they did a number of different studies on human reaction times, where they measured the Perceiving, Thinking, and Acting phases in the Brain, the test results range as follows:

Player side(Brain)：

Perceiving: Perceptual Processor: ~100ms \[50-200ms\]

Thinking: Cognitive Processor: ~70ms \[30-100ms\]

Acting: Motor Processor: ~70ms \[25-170ms\]

It should be noted that players need to identify, associate, and memorize the pose and the attack when facing new moves, so the whole process will take longer than this, but the time will be shortened as players face this move many times and form memories. Anyway, this response data will be used as the basic measure in the following article. (For those interested, please refer to [this](<https://cio-wiki.org/wiki/Model_Human_Processor_(MHP)>) )

For the computer's response time, Mick West (Programmer of the original Tony Hawk mechanic) notes that games with a response time of 50 to 100ms typically feel "Tight" and "Responsive", this is because 50 to 100ms happens to be within a cycle of a player's Perceptual Processor, which is 100ms. If the response is more than 100ms the delay will be noticeable, and if it reaches about 200ms, it will feel sluggish. Monster Hunter: Rise, Halo: Infinite, and Hollow Knight all have response times inferior to 100ms.

Computer Side (Processor):

Game System Processor: <100mm

My test result of Dark Souls 3's input response time is around 60-100ms.

An Interaction in Dark Souls 3

Next, we apply the interaction model to the game. Let's assume a player who is familiar with Black Knight's moveset, and analyze his thinking process of using Roll to evade Black Knight's attack. (Roll's Invincible window is active at the first frame of the animation and it lasts for 433ms (Medium Roll))

![](https://eu-images.contentstack.com/v3/assets/blt740a130ae3c5d529/bltec50e317ef824c94/650f0cadde5e9f65a552e0da/52103231977_1649e73360_o.png/?width=700&auto=webp&quality=80&disable=upscale)

In this interaction, we see the phases of the Black Knight's attack animation are clear enough and provide enough reaction time, plus the instant input response, all of these together lay the solid foundation for the fairness of the combat. Moreover, with this method, we can disassemble the player's thinking process, and then deduce the design intention behind the enemy animation.

The design behind the attack

After analyzing enemy attack animations and the combat experience with the above method, we can try to summarize the design behind each attack phase.

Opening Pose

Basic rules:

• Its silhouette should be discernable and naturally hints the incoming attack.

• It needs to express the weight and inertia of the attack.

Difficulty adjustment:

• Multiple attack actions use very similar opening pose with different duration. Players need to tell the difference by observing the nuance of animation and the hints from SFX, VFX, or any other channel.

Attack Signal

Basic rules:

• Attack Signal and Attack together need to have a guaranteed duration of at least 340ms, but as mentioned above, if the player is facing a new move, this duration will be required to be longer

• Appropriately bend the physics in attack motions to make the player feel attack is instant and snappy

Difficulty adjustment:

• It is very unfortunate that Dark Souls 3 increased the difficulty by shortening the duration to less than 340ms so that the player cannot react in time (yes, that was it), but this method is generally only used for few moves of a small number of enemies, such as the first Hollow at the High Wall of Lothric, some of his attacks are almost impossible to dodge. I often underestimate these early enemies in the later stages of the game, and I always come a cropper (^-^!), however, because these monsters are very fragile and easy to be staggered, so it seems that there is a balance consideration when using this. But in a word, you can't ignore any enemy in the Dark Souls franchise.

​Attack

Basic rules:

• The key to this phase is to set the hitbox. In Dark Souls 3, most of the weapon hitboxes are several times the size of the weapon model, and it is even bigger when applied to monsters' limb, however, from the perspective of gameplay clarity the expansion will bring a negative effect as the player can not accurately recognize the damage range, so why they still do it?

• From a technical point of view, because the animation speed in the attack phase is extremely fast, and the hitbox is only active in 2-4 frames, it is likely to miss the collision or damage detection between the frame if it is the same size as the weapon.

• From a design point of view, to strengthen the power fantasy, take Darkeater Midir as an example, expand its attack range means increasing the difficulty, and then enhancing the power fantasy. In addition, because its limbs are used as weapons, so the attack animation is limited by the model, the bone structure, body mechanics, and artistic value. If the hitbox is not enlarged and only set accurately to limb's size, it will be difficult for the attack to hit the player. Just imagine, such a powerful attack misses the hit to me who is standing still in front of him, then the power fantasy will be broken up for me.

![](https://eu-images.contentstack.com/v3/assets/blt740a130ae3c5d529/blt7c90f95f40e8a35f/650f12271f554f11b2e40d0d/52101975113_6610f726d1_b.jpg/?width=700&auto=webp&quality=80&disable=upscale)

(Figure 3)The hitbox size of the claw swipe is several times the size of the claw

• Setting the hitbox size is a very tricky and risky task as it can easily break the power fantasy if you set it inappropriately. The method adopted in Dark Souls 3, one is to make full use of instantaneity of attacks, making it difficult for players to notice at the moment when they are hit, and the other is to appropriately set the active frame and deactivate frame of the hitbox.

End Pose

Basic rules:

• This phase can be short or skipped if it is followed by an attack in a combo.

• If this stage is used as a Bait to seduce the player to act, the duration must be at least 240ms, and the longer the time, the more significant the effect of Bait is.

• If it is followed by Return and giving the WoO, the duration must be at least 170ms for the player to confirm the end of this attack.

Difficulty adjustment:

• How many times the attack is followed by Return, which means how many times the player is given WoO, the fewer times, the greater the difficulty.

• The longer the duration of Bait is set, the higher the probability of the player being hooked, and the greater the difficulty.

Return

Basic rules:

• It is WoO phase, generally the longest among the 5 phases

Difficulty adjustment:

• The length of this phase is determined by the strength of the attack and the overall difficulty balance of the enemy. The shorter the time, the higher the difficulty

Summary

This article analyzes how Dark Souls 3 achieves combat clarity and fairness by breaking down an enemy attack and applying the interaction model. Moreover, it also tries to establish a framework, which can be used to analyze other action games or a tool to create your own game's combat style.

In addition to animation, the SFX, VFX, Voice, etc. are important expression channels in combat as well, especially the SFX, many players rely on sound to memorize the rhythm and pattern of the boss's attack, and that's why the blindfolded boss challenges are possible.

Hope to see more research articles on different expression channels of Souls game!

If you have interest in this article, please feel free to reach out to [me](mailto:yongliangz7@gmail.com) or my [discord](https://discordapp.com/users/446192755371409409), I will be glad to hear from your.

References:

1.  Steve Swint, Game Feel, Morgan Kaufmann Publishers, 2009
2.  Stuart K. Card, Thomas P. Moran, & Allen Newell, Model Human Processor（Handbook of Perception and Human Performance）, Wiley-Interscience, 1986
