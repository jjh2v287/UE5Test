# Narxim's GAS Example
### A bare-bones Gameplay Ability System (GAS) example project to help you get started

![GitHub Repo stars](https://img.shields.io/github/stars/Narxim/Narxim-GAS-Example?style=flat-square)
![GitHub last commit](https://img.shields.io/github/last-commit/Narxim/Narxim-GAS-Tutorial?style=flat-square)
![GitHub](https://img.shields.io/github/license/Narxim/Narxim-GAS-Example?style=flat-square)
![Unreal](https://img.shields.io/badge/Unreal_Engine_Versions-_(4.26+)_(5.0+)_Current_Version_(5.5)_-informational?style=flat-square)

![image](https://github.com/user-attachments/assets/2518ffc8-ef3b-4bbf-bcce-48d00338237a)

## A fully-setup example project!
___

GAS has a lot of moving parts and can be quite daunting to wrap your head around at first glance.
I am providing this example project as a basis for learning how to properly set up your project to utilize all of its components.

There are obviously many ways to set up your project, but I believe this example provides a simple, versatile base to build upon, and especially to understand GAS as a whole.

This project is currently being updated, same for the documentation. Please be patient :)

---

<p>Note: The 5.4 update of this project has introduced significant changes in an attempt to simplify the setup and add some commonly used / requested features!</p>
___

### Includes:
- Thoroughly commented C++ code
- Example Blueprints
- Sample Attribute sets: Health (Includes Damage / Healing), Stamina,  Leveling and Resistance
- Stamina / Health Regeneration example (Based on Regeneration Attribute)
- "Ability System Initialization Data" structure for initializing the Ability System Component
- Ability Trigger / Release bindings tied directly to Enhanced Input (example)
- Commonly used example Ability Tasks
- Native Gameplay Tags example (NativeGameplayTags.h)
- Example UI elements with Attribute event bindings (In Game UI / Nameplate)
- Sample abilities
- Player Character and Non Player Character class examples
- Effect samples (Damage, armor buff/debuff, fire armor/damage, bleed status ...)
___

#### Check out the **[Unreal Source Discord](https://discord.gg/unrealsource)** if you have any questions!
---
## Features
### Native gameplay tags
[Gameplay tags defined in C++](https://github.com/Narxim/Narxim-GAS-Example/blob/master/Source/GAS_Example/AbilitySystem/Data/NativeGameplayTags.h) only, without the need to declare them in DefaultGameplayTag.ini
___
### Attribute Sets
We are providing different attribute sets, with a BP implementation for each.

![image](https://github.com/user-attachments/assets/7b7146fd-9e43-4921-a85c-7064595d0f2b)

That's a best practice (Based on working experience), that's not only for ATs, but for any C++ class that might get reference by another object.

If you have a BP implementation, and for example, you add some config variable on your C++ class, you'll be able to adjust them in the editor without even changing the reference from the C++ class to the new BP.
___
#### Health
This attribute set comes with:
- Current Health
- Maximum Health
- Health Regeneration

Current cannot go over Max, and will be proportionnaly adjusted if the Maximum is changing (down or up).

Adjustments are done using a convenience method on the Base attribute set, called in PostAttributeChange (which is called whenever an attribute is changing, from both temporary and base adjustments).

Health Regeneration is used by an infinite GE, granting health back per second.

Once Current Health attains the Maximum, we'll add a 
___
#### Stamina

Stamina is implemented in the same way as Health (Current / Max), with proportional adjustments, and regeneration.
Example for Stamina Regen:

![image](https://github.com/user-attachments/assets/7357ef50-10a3-4a04-a0cd-088ccf3dc02a)

___
#### Leveling System

Press NumPad -/+ (or Up/Down) to Level up your character.
This will reevaluate all Attribute initializers and adjust the Maximum atttributes, adjusting the Current values that might be linked.

All GAs and GEs will also have their levels updated (Except if they have the Effect.NoLevel tag).

GEs using Curve tables with multiple levels in them will now use the right leveled value.
___
#### Resistance

We've implemented LoL (League of Legends) Armor system:
- Resistance is based on 100
- Resistance attribute can go from -75 to +200
- Damage reduction is calculated as 100 / (100 - Resistance attribute)
- More armor will yield less return, while less that 100 will increase the damage.

We provide one Attribute set with one attribute: Resistance.
And with this one attribute, every type of damage can have their own resistance.
See [KaosSpectrum's idea of Armor / Resistance](https://www.thegames.dev/?p=165) for more information.

The UI will showcase how to access each resistance individually, with the in-game effect, live:

![image](https://github.com/user-attachments/assets/b30e03a3-745b-450d-8237-1d825c7c9fa1)

Middle is the current Resistance magnitude (0 - 25).
Top, showing the effective resistance, calculated with the previous formula.

The slider will show if it is good or bad, with the min/max.
___
### Ability System Initialization Data can be found in the Character class Blueprints.

![image](https://github.com/user-attachments/assets/af28429d-fac8-4efa-97d2-9fcce0b4aef5)

You can specify:
- Which attribute sets to grant
- Initialize each attribute from a hard coded value or a Curve Float table ("CSV")
- Starting Gameplay Abilities
- Starting Gameplay effects
___
### Damage

The project comes with:
- Basic Direct Damage (Physical damage)
- Fire Damage
- Bleed Damage (Damage over Time / DoT)
___
### Resistance and Damage handling

All damage _should_ be applied server authoritative, through an execution, if possible.
The reason is simple: a game should avoid rubber-banding death. That's not something you can get away with.

We're providing multiple Gameplay Effects damage examples, all using [GameplayEffectExecutionCalculations](https://github.com/tranek/GASDocumentation?tab=readme-ov-file#4512-gameplay-effect-execution-calculation) (or called Execution, Exec cals) in 2 flavors:
- Simple version
- Advanced version

Any of those Exec can access:
- Source and Target Ability System Component
- Source and Target actors
- Source and Target tags
- GE's context and spec

![image](https://github.com/user-attachments/assets/3965911e-3d25-417e-9e1c-83cab978476a)
___
#### Simple damage calculation

That's the [boiler plate implementation](https://github.com/Narxim/Narxim-GAS-Example/blob/master/Source/GAS_Example/AbilitySystem/Calculations/SimpleDamageGameplayEffectExecutionCalculation.cpp) for our project, using:
- One meta attribute called "Damage" that will be applied as "real damage" to Current Health.

The "Resistance" we talked about earlier is actually implemeted exactly as KaosSpectrum explained.
This version has a small downside though: You cannot applied more than one type of damage per Gameplay effect.
The reason: GE's asset tag will be used to calculate the right Resistance.

That's how our "Fire" Damage is implemented:
- Simple execution
- GE and some tags
___
#### Advanced damage calculation

This ["Advanced"](https://github.com/Narxim/Narxim-GAS-Example/blob/master/Source/GAS_Example/AbilitySystem/Calculations/DamageGameplayEffectExecutionCalculation.cpp) one allows for more than one damage type while retaining the ability.

The Exec will capture 2 damage attributes (Damage and ReceivedBleeding).
Inside the exec, the "same code" from the simple version will be ran, for each type of damage, but the Tags will be provided dynamically in there.

The GE doesn't have to provide any tags, all is handled in the Exec.

We also think it would be possible to handle multiple damage type using only one meta attribute, but for the sack of simplicity, 2 are used here.
___
### Test Volumes

This project provides several test volumes that are applying each different gameplay effects.

Feel free to have a look at the video
[![image](https://github.com/user-attachments/assets/baedccee-45bb-43bd-a761-1966ca614c1d)](https://www.youtube.com/watch?v=QW6GwY8DV94)


#### Healing Volumes
![image](https://github.com/user-attachments/assets/4ae4cd95-e493-4f0e-af67-b70cd82b114d)

#### Armor and Damage volumes
![image](https://github.com/user-attachments/assets/4a4495b8-c876-4e01-a575-1bc8764822fc)

We are using only one Resistance Attribute.

#### Bleed Status (DoT)
![image](https://github.com/user-attachments/assets/f1727235-cf66-4805-a1b8-6bc7b2bf9bff)

We are using only one Resistance Attribute.

#### Fire Damage and Fire Resistance
![image](https://github.com/user-attachments/assets/66a1dd44-dbc7-4b2c-8468-36d7a2b28013)

We are using only one Resistance Attribute.
___
### Showing Gameplay effects on the UI

![image](https://github.com/user-attachments/assets/c7452ef6-72b5-428d-8982-0fe26c73f8c7)

We just released a basic implementation of Effect UI elements, as seen previously.
The concept is very simple.

![image](https://github.com/user-attachments/assets/01d29705-8a77-4f76-baf6-aef1d17fba44)

It starts with Gameplay effect having a special component:

![image](https://github.com/user-attachments/assets/55a729d9-c19e-4797-945b-d5178d36bd47)

This component allows developper to say "I want to show this Effect to the player" with some parameters:
- Title
- Description
- Icon / Material
- Specific Controller

The ASC has been modded with a new delegate:
```
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCustomGameplayEffectEventDelegate, const ECustomEffectEventType, EventType, const FActiveGameplayEffect&, Effect);
FOnCustomGameplayEffectEventDelegate OnCustomGameplayEffectEventDelegate;
```
It is called whenever an effect - with a UI Component - is applied to you.

UIs can now subscribe to this delegate, and that's what we've done with the UAbilitySystemWidget, with a Blueprint Effect bar

![image](https://github.com/user-attachments/assets/43c32e3a-8fb7-4e89-a0f1-03e709eeb893)

Once the Widget receives the event, it will create, or reuse, a controller.
The controller is there to create the Effect Widget, and control ALL of its parameters whenever something changed.

![image](https://github.com/user-attachments/assets/eb868be1-cc3a-4727-b7fa-6671cb1ebb39)

A basic implementation is delivered:

![image](https://github.com/user-attachments/assets/80865215-43c8-494b-85d1-65b19ff3fd2b)
![image](https://github.com/user-attachments/assets/fe685d31-5e24-4920-b245-b77615f22793)

The basic functions are implemented:
- Updating Title, Desciption, Icon (on init)
- Updating Time and stack (Periodically)
But ... you can override them in C++ or BP if you wish.

![image](https://github.com/user-attachments/assets/cf74a356-f9ea-4819-bd44-c8725d05727b)

With a basic Widget class

![image](https://github.com/user-attachments/assets/ce3cc672-3feb-4c06-becf-734f11ddbec6)
![image](https://github.com/user-attachments/assets/a4cec93c-480a-48ab-9411-25bfb6161909)

The Controller will spawn the Widget class it has (Base, or any you might want to create) and take ownership of it.

If the effect is removed, the controller will remove the widget and gets destroyed.
More stacks ? Update the stacks ...
Inhibition is also not taken in account (Widget will be dimmed).

The controller will be listening to those updates from the ActiveEffect Event sets:
```
// Subscribe to all available events on the gameplay effect event set, so that the controller can respond accordingly
	FActiveGameplayEffectEvents* const EventSet = ASC->GetActiveEffectEventSet(Handle);
	EventSet->OnEffectRemoved.AddUObject(this, &ThisClass::OnEffectRemoved);
	EventSet->OnStackChanged.AddUObject(this, &ThisClass::OnStackChanged);
	EventSet->OnInhibitionChanged.AddUObject(this, &ThisClass::OnInhibitionChanged);
	EventSet->OnTimeChanged.AddUObject(this, &ThisClass::OnTimeChanged);
```
___
## ChangeLog:
### 2025/01/20 (EvilHippo):
```
- Project is now available and setup for 5.5
- Added more features to UI Effect Bar: Inhibition, Duration, Stacks etc ....
- Added UE5Coro (not enabled by default). Requires MSVC 14.41+
- Added a "Very simple" Ranged Attack (Left Click) dealing damage.
- Added New Volume to "Inhibit" Fire resistance, and show that the UI Effect will be dimmed (As in Inhibited)
- Making more method BlueprintNativeEvent
```
### 2025/01/06 (EvilHippo):
```
- Adding a new Custom GE Component that can have custom CanApply in BP or C++
- Create a new GE Component to limit max instances of GEs without stacking policies
- Create a test volume for this new Component.
```

### 2025/01/05 (EvilHippo):
```
- Now shows Effects on the UI as long as a Custom UI Component has been added.
- Shows Title, Desc, Duration (updated) and stack count
- Inhibition is not yet shown
- Added a bunch of free icons (see Licence in Content/UI/Icons)
- Added a new Widget Effect Bar + Effect Widget + Effect Widget Controller. Have to add some more example.
```

### 2024/12/28 (EvilHippo):
```- Using one Resistance Attribute for all "Armors".
- Introduced Fire Damage, with a simple calculation exec
Simple: Simple Exec, no fancy things, everything handled with tags on GE "Base Kaos style"
Advanced: can specify more than one damage at the same time while retaining the same feature, and Damage GE doesn't have to have any specific tags
- Introduced Attribute Maxed out tag, to be able to drive regen (stop if already at max), or be able to do some other type of action if Maxed out (Like: Damage is increased if target has nax health ?)
- Reworked some C++ functions
- Created BP Implementation of all Attribute sets (Best practice: never use the C++ class directly in other objects)
- Created a Base Custom Ability
- Renamed Volumes and GEs associated with them
- Introduced new Resistance UI (showing off how you can get the right value for each resistance with only ONE attribute)
- Added some Gameplay cues for Damage / Armor / Fire / Bleeding
```

### 2024/12/19 (EvilHippo):
```General:
- Adjusted C++ (removing Pyramids, refactoring some methods)
- Added TEXT() Macro
- Replaced ASC in BaseCharacter with Custom ASC pointer.
- Renamed AbilitySystemFunctionLibrary to CustomAbilitySystemBlueprintLibrary
- Changed ASC Initializer arrays to sets (better handling when subclassing)
- Enabled replication push model (better network performance, easier control)

Gameplay:
- Added Levels (+AbilitySystemWidget update)
Can now use NumPad -/+ to increase the character level
Tag "NoLevel" can be used on GE that shouldn't be updated.
Level will be shown on the UI (Nameplate + HUD).
Levels will adjust values in GAs, GEs and Attributes that are using Scalable float with Curve floats (instead of hardcoded values)

- Added Curves floats for Attribute initialization (replaces floats with Scalable Floats)
Using CSVs and automatic Curve float update (Just change the CSV, the Curve will be updated automatically)

- Created BP implementation of the Attribute Sets:
Best practice: Try as much as possible to use BP instances instead of classes if reference somewhere.
```
