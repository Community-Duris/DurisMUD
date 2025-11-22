/* *************************************************************************
 *  file: Interpreter.h , Command interpreter module.      Part of Duris *
 *  Usage: Procedures interpreting user command                            *
 ************************************************************************* */

#ifndef _SOJ_INTERP_H_
#define _SOJ_INTERP_H_

#define ACMD(c)  \
   void (c)(P_char ch, char *argument, int cmd)

#define LOCK_NONE                   0
#define LOCK_CREATION           BIT_1
#define LOCK_CONNECTIONS        BIT_2
#define LOCK_MAX_PLAYERS        BIT_3
#define LOCK_LEVEL              BIT_4

extern unsigned int game_locked;
extern unsigned int game_locked_players;
extern unsigned int game_locked_level;

/* these defines are here to facilitate adding/deleting/reordering commands.
   Especially for things like specials looking for specific commands

   NOTE!  the number assigned to the CMD_ macros MUST match the position of the
   corresponding word in the *command[] list:

   CMD_KISS is 25, "kiss" must be the 25th string in *commands[] (interp.c)
   */

#define CMD_NONE             0
#define CMD_NORTH            1
#define CMD_EAST             2
#define CMD_SOUTH            3
#define CMD_WEST             4
#define CMD_UP               5
#define CMD_DOWN             6
#define CMD_ENTER            7
#define CMD_EXITS            8
#define CMD_KILL             9
#define CMD_GET             10
#define CMD_DRINK           11
#define CMD_EAT             12
#define CMD_WEAR            13
#define CMD_WIELD           14
#define CMD_LOOK            15
#define CMD_SCORE           16
#define CMD_SAY             17
#define CMD_GSHOUT          18
#define CMD_TELL            19
#define CMD_INVENTORY       20
#define CMD_QUI             21
#define CMD_BOUNCE          22
#define CMD_SMILE           23
#define CMD_DANCE           24
#define CMD_KISS            25
#define CMD_CACKLE          26
#define CMD_LAUGH           27
#define CMD_GIGGLE          28
#define CMD_SHAKE           29
#define CMD_PUKE            30
#define CMD_GROWL           31
#define CMD_SCREAM          32
#define CMD_INSULT          33
#define CMD_COMFORT         34
#define CMD_NOD             35
#define CMD_SIGH            36
#define CMD_SULK            37
#define CMD_HELP            38
#define CMD_WHO             39
#define CMD_EMOTE           40
#define CMD_ECHO            41
#define CMD_STAND           42
#define CMD_SIT             43
#define CMD_REST            44
#define CMD_SLEEP           45
#define CMD_WAKE            46
#define CMD_FORCE           47
#define CMD_TRANSFER        48
#define CMD_HUG             49
#define CMD_SNUGGLE         50
#define CMD_CUDDLE          51
#define CMD_NUZZLE          52
#define CMD_CRY             53
#define CMD_NEWS            54
#define CMD_EQUIPMENT       55
#define CMD_BUY             56
#define CMD_SELL            57
#define CMD_VALUE           58
#define CMD_LIST            59
#define CMD_DROP            60
#define CMD_GOTO            61
#define CMD_WEATHER         62
#define CMD_READ            63
#define CMD_POUR            64
#define CMD_GRAB            65
#define CMD_REMOVE          66
#define CMD_PUT             67
#define CMD_SHUTDOW         68
#define CMD_SAVE            69
#define CMD_HIT             70
#define CMD_STRING          71
#define CMD_GIVE            72
#define CMD_QUIT            73
#define CMD_STAT            74
#define CMD_INNATE          75
#define CMD_TIME            76
#define CMD_LOAD            77
#define CMD_PURGE           78
#define CMD_SHUTDOWN        79
#define CMD_IDEA            80
#define CMD_TYPO            81
#define CMD_BUG             82
#define CMD_WHISPER         83
#define CMD_CAST            84
#define CMD_AT              85
#define CMD_ASK             86
#define CMD_ORDER           87
#define CMD_SIP             88
#define CMD_TASTE           89
#define CMD_SNOOP           90
#define CMD_FOLLOW          91
#define CMD_RENT            92
#define CMD_OFFER           93
#define CMD_POKE            94
#define CMD_ADVANCE         95
#define CMD_ACC             96
#define CMD_GRIN            97
#define CMD_BOW             98
#define CMD_OPEN            99
#define CMD_CLOSE          100
#define CMD_LOCK           101
#define CMD_UNLOCK         102
#define CMD_MREPORT        103
#define CMD_APPLAUD        104
#define CMD_BLUSH          105
#define CMD_BURP           106
#define CMD_CHUCKLE        107
#define CMD_CLAP           108
#define CMD_COUGH          109
#define CMD_CURTSEY        110
#define CMD_FART           111
#define CMD_FLIP           112
#define CMD_FONDLE         113
#define CMD_FROWN          114
#define CMD_GASP           115
#define CMD_GLARE          116
#define CMD_GROAN          117
#define CMD_GROPE          118
#define CMD_HICCUP         119
#define CMD_LICK           120
#define CMD_LOVE           121
#define CMD_MOAN           122
#define CMD_NIBBLE         123
#define CMD_POUT           124
#define CMD_PURR           125
#define CMD_RUFFLE         126
#define CMD_SHIVER         127
#define CMD_SHRUG          128
#define CMD_SING           129
#define CMD_SLAP           130
#define CMD_SMIRK          131
#define CMD_SNAP           132
#define CMD_SNEEZE         133
#define CMD_SNICKER        134
#define CMD_SNIFF          135
#define CMD_SNORE          136
#define CMD_SPIT           137
#define CMD_SQUEEZE        138
#define CMD_STARE          139
#define CMD_STRUT          140
#define CMD_THANK          141
#define CMD_TWIDDLE        142
#define CMD_WAVE           143
#define CMD_WHISTLE        144
#define CMD_WIGGLE         145
#define CMD_WINK           146
#define CMD_YAWN           147
#define CMD_SNOWBALL       148
#define CMD_WRITE          149
#define CMD_HOLD           150
#define CMD_FLEE           151
#define CMD_SNEAK          152
#define CMD_HIDE           153
#define CMD_BACKSTAB       154
#define CMD_PICK           155
#define CMD_STEAL          156
#define CMD_BASH           157
#define CMD_RESCUE         158
#define CMD_KICK           159
#define CMD_FRENCH         160
#define CMD_COMB           161
#define CMD_MASSAGE        162
#define CMD_TICKLE         163
#define CMD_PRACTICE       164
#define CMD_PAT            165
#define CMD_EXAMINE        166
#define CMD_TAKE           167
#define CMD_INFO           168
#define CMD_SPELLS         169
#define CMD_PRACTISE       170
#define CMD_CURSE          171
#define CMD_USE            172
#define CMD_WHERE          173
#define CMD_LEVELS         174
#define CMD_REROLL         175
#define CMD_PRAY           176
#define CMD_EMOTE2         177        /* ":" */
#define CMD_BEG            178
#define CMD_BLEED          179
#define CMD_CRINGE         180
#define CMD_DREAM          181
#define CMD_FUME           182
#define CMD_GROVEL         183
#define CMD_HOP            184
#define CMD_NUDGE          185
#define CMD_PEER           186
#define CMD_POINT          187
#define CMD_PONDER         188
#define CMD_PUNCH          189
#define CMD_SNARL          190
#define CMD_SPANK          191
#define CMD_STEAM          192
#define CMD_GROUND         193
#define CMD_TAUNT          194
#define CMD_THINK          195
#define CMD_WHINE          196
#define CMD_WORSHIP        197
#define CMD_YODEL          198
#define CMD_TOGGLE         199
#define CMD_WIZMSG         200
#define CMD_CONSIDER       201
#define CMD_GROUP          202
#define CMD_RESTORE        203
#define CMD_RETURN         204
#define CMD_SWITCH         205
#define CMD_QUAFF          206
#define CMD_RECITE         207
#define CMD_USERS          208
#define CMD_POSE           209
#define CMD_SILENCE        210
#define CMD_WIZHELP        211
#define CMD_CREDITS        212
#define CMD_DISBAND        213
#define CMD_VIS            214
#define CMD_LFLAGS         215
#define CMD_POOFIN         216
#define CMD_WIZLIST        217
#define CMD_DISPLAY        218
#define CMD_ECHOA          219
#define CMD_DEMOTE         220
#define CMD_POOFOUT        221
#define CMD_CIRCLE         222
#define CMD_BALANCE        223
#define CMD_WIZLOCK        224
#define CMD_DEPOSIT        225
#define CMD_WITHDRAW       226
#define CMD_IGNORE         227
#define CMD_SETATTR        228
#define CMD_TITLE          229
#define CMD_AGGR           230
#define CMD_GSAY           231
#define CMD_CONSENT        232
#define CMD_SETBIT         233
#define CMD_HITALL         234
#define CMD_TRAP           235
#define CMD_MURDER         236
#define CMD_GLANCE         237
#define CMD_AUCTION        238
#define CMD_CHANNEL        239
#define CMD_FILL           240
#define CMD_GCC            241
#define CMD_TRACK          242
#define CMD_PAGE           243
#define CMD_COMMANDS       244
#define CMD_ATTRIBUTES     245
#define CMD_RULES          246
#define CMD_ANALYZE        247
#define CMD_LISTEN         248
#define CMD_DISARM         249
#define CMD_PET            250
#define CMD_DELETE         251
#define CMD_BAN            252
#define CMD_ALLOW          253
#define CMD_PLAY           254
#define CMD_MOVE           255        /* first chessboard command */
#define CMD_BRIBE          256
#define CMD_BONK           257
#define CMD_CALM           258
#define CMD_RUB            259
#define CMD_CENSOR         260
#define CMD_CHOKE          261
#define CMD_DROOL          262
#define CMD_FLEX           263
#define CMD_JUMP           264
#define CMD_LEAN           265
#define CMD_MOON           266
#define CMD_OGLE           267
#define CMD_PANT           268
#define CMD_PINCH          269
#define CMD_PUSH           270
#define CMD_SCARE          271
#define CMD_SCOLD          272
#define CMD_SEDUCE         273
#define CMD_SHOVE          274
#define CMD_SHUDDER        275
#define CMD_SHUSH          276
#define CMD_SLOBBER        277
#define CMD_SMELL          278
#define CMD_SNEER          279
#define CMD_SPIN           280
#define CMD_SQUIRM         281
#define CMD_STOMP          282
#define CMD_STRANGLE       283
#define CMD_STRETCH        284
#define CMD_TAP            285
#define CMD_TEASE          286
#define CMD_TIP            287
#define CMD_TWEAK          288
#define CMD_TWIRL          289
#define CMD_UNDRESS        290
#define CMD_WHIMPER        291
#define CMD_EXCHANGE       292
#define CMD_RELEASE        293
#define CMD_SEARCH         294
#define CMD_JOIN           295
#define CMD_CAMP           296
#define CMD_SECRET         297
#define CMD_LOOKUP         298
#define CMD_REPORT         299
#define CMD_SPLIT          300
#define CMD_WORLD          301
#define CMD_JUNK           302
#define CMD_PETITION       303
#define CMD_DO             304
#define CMD_SAY2           305        /* "'" */
#define CMD_CARESS         306
#define CMD_BURY           307
#define CMD_DONATE         308
#define CMD_SHOUT          309
#define CMD_DISEMBARK      310
#define CMD_PANIC          311
#define CMD_NOG            312
#define CMD_TWIBBLE        313
#define CMD_THROW          314
#define CMD_LEGSWEEP       315
#define CMD_SWEEP          316 // Tailsweep
#define CMD_APOLOGIZE      317
#define CMD_AFK            318
#define CMD_LAG            319
#define CMD_TOUCH          320
#define CMD_SCRATCH        321
#define CMD_WINCE          322
#define CMD_TOSS           323
#define CMD_FLAME          324
#define CMD_ARCH           325
#define CMD_AMAZE          326
#define CMD_BATHE          327
#define CMD_EMBRACE        328
#define CMD_BRB            329
#define CMD_ACK            330
#define CMD_CHEER          331
#define CMD_SNORT          332
#define CMD_EYEBROW        333
#define CMD_BANG           334
#define CMD_PILLOW         335
#define CMD_NAP            336
#define CMD_NOSE           337
#define CMD_RAISE          338
#define CMD_HAND           339
#define CMD_PULL           340
#define CMD_TUG            341
#define CMD_WET            342
#define CMD_MOSH           343
#define CMD_WAIT           344
#define CMD_HI5            345
#define CMD_ENVY           346
#define CMD_FLIRT          347
#define CMD_BARK           348
#define CMD_WHAP           349
#define CMD_ROLL           350
#define CMD_BLINK          351
#define CMD_DUH            352
#define CMD_GAG            353
#define CMD_GRUMBLE        354
#define CMD_DROPKICK       355
#define CMD_WHATEVER       356
#define CMD_FOOL           357
#define CMD_NOOGIE         358
#define CMD_MELT           359
#define CMD_SMOKE          360 // Smoking dat herb.
#define CMD_WHEEZE         361
#define CMD_BIRD           362
#define CMD_BOGGLE         363
#define CMD_HISS           364
#define CMD_BITE           365
#define CMD_TELEPORT       366
#define CMD_BANDAGE        367
#define CMD_BLOW           368
#define CMD_BORED          369
#define CMD_BYE            370
#define CMD_CONGRATULATE   371
#define CMD_DUCK           372
#define CMD_FLUTTER        373
#define CMD_GOOSE          374
#define CMD_GULP           375
#define CMD_HALO           376
#define CMD_HELLO          377
#define CMD_HICKEY         378
#define CMD_HOSE           379
#define CMD_HUM            380
#define CMD_IMPALE         381
#define CMD_JAM            382
#define CMD_KNEEL          383
#define CMD_MOURN          384
#define CMD_PROTECT        385
#define CMD_PUZZLE         386
#define CMD_ROAR           387
#define CMD_ROSE           388
#define CMD_SALUTE         389
#define CMD_SKIP           390
#define CMD_SWAT           391
#define CMD_TONGUE         392
#define CMD_WOOPS          393
#define CMD_ZONE           394
#define CMD_TRIP           395
#define CMD_MEDITATE       396
#define CMD_SHAPECHANGE    397
#define CMD_ASSIST         398
#define CMD_DOORBASH       399
#define CMD_EXP            400
#define CMD_EXPKKK         401  // Arih: for debugging exp bug
#define CMD_ROFL           402
#define CMD_AGREE          403
#define CMD_HAPPY          404
#define CMD_PUCKER         405
#define CMD_SPAM           406
#define CMD_BEER           407
#define CMD_BODYSLAM       408
#define CMD_SACRIFICE      409
#define CMD_TERMINATE      410
#define CMD_CD             411 // For labelas proc
#define CMD_MEMORIZE       412
#define CMD_FORGET         413
#define CMD_HEADBUTT       414
#define CMD_SHADOW         415
#define CMD_RIDE           416
#define CMD_MOUNT          417
#define CMD_DISMOUNT       418
#define CMD_DEBUG          419
#define CMD_FREEZE         420
#define CMD_BBL            421
#define CMD_GAPE           422
#define CMD_VETO           423
#define CMD_JK             424
#define CMD_TIPTOE         425
#define CMD_GRUNT          426
#define CMD_HOLDON         427
#define CMD_IMITATE        428
#define CMD_TANGO          429
#define CMD_TARZAN         430
#define CMD_POUNCE         431
#define CMD_CHEEK          432
#define CMD_LAYHAND        433
#define CMD_AWARENESS      434
#define CMD_FIRSTAID       435
#define CMD_SPRINGLEAP     436
#define CMD_FEIGNDEATH     437
#define CMD_CHANT          438
#define CMD_DRAG           439
#define CMD_SPEAK          440
#define CMD_RELOAD         441
#define CMD_DRAGONPUNCH    442
#define CMD_REVOKE         443
#define CMD_GRANT          444
#define CMD_OLC            445 // removed
#define CMD_MOTD           446
#define CMD_ZRESET         447
#define CMD_FULL           448
#define CMD_WELCOME        449
#define CMD_INTRODUCE      450
#define CMD_SWEAT          451
#define CMD_MUTTER         452
#define CMD_LUCKY          453
#define CMD_AYT            454
#define CMD_FIDGET         455
#define CMD_FUZZY          456
#define CMD_SNOOGIE        457
#define CMD_READY          458
#define CMD_PLONK          459
#define CMD_HERO           460
#define CMD_LOST           461
#define CMD_DRAIN          462
#define CMD_FLASH          463
#define CMD_CURIOUS        464
#define CMD_HUNGER         465
#define CMD_THIRST         466
#define CMD_ECHOZ          467
#define CMD_PTELL          468
#define CMD_SCRIBE         469
#define CMD_TEACH          470
#define CMD_REINITPHYS     471
#define CMD_FINGER         472
#define CMD_APPROVE        473
#define CMD_DECLINE        474
#define CMD_SUMMON         475
#define CMD_CLONE          476
#define CMD_TROPHY         477
#define CMD_ZAP            478 // Special triger?
#define CMD_ALERT          479
#define CMD_RECLINE        480
#define CMD_KNOCK          481
#define CMD_SKILLS         482
#define CMD_BERSERK        483
#define CMD_FAQ            484
#define CMD_DISENGAGE      485
#define CMD_RETREAT        486
#define CMD_INROOM         487
#define CMD_WHICH          488
#define CMD_PROCLIB        489
#define CMD_SETHOME        490
#define CMD_SCAN           491
#define CMD_APPLY          492 // Poison
#define CMD_HITCH          493 // Cart 'n Horse
#define CMD_UNHITCH        494
#define CMD_MAP            495
#define CMD_OGRE_ROAR      496
#define CMD_BEARHUG        497
#define CMD_DIG            498
#define CMD_JUSTICE        499
#define CMD_SUPERVISE      500
#define CMD_SOCIETY        501
#define CMD_TRAPSET        502
#define CMD_TRAPREMOVE     503
#define CMD_CARVE          504
#define CMD_DEPISS         505
#define CMD_REPISS         506
#define CMD_BREATH         507
#define CMD_INGAME         508
#define CMD_FIRE           509
#define CMD_REPAIR         510
#define CMD_MAIL           511
#define CMD_RENAME         512
#define CMD_PROJECT        513
#define CMD_ABSORBE        514
#define CMD_FLY            515
#define CMD_SQUIDRAGE      516
#define CMD_FRAGLIST       517
#define CMD_LWITNESS       518
#define CMD_PAY            519
#define CMD_PARDON         520
#define CMD_REPORTING      521
#define CMD_BIND           522
#define CMD_UNBIND         523
#define CMD_TURN_IN        524
#define CMD_CAPTURE        525
#define CMD_APPRAISE       526
#define CMD_COVER          527
#define CMD_HOUSE          528
#define CMD_GUILDHALL      529
#define CMD_DOORKICK       530
#define CMD_BUCK           531
#define CMD_STAMPEDE       532
#define CMD_SUBTERFUGE     533
#define CMD_DIRTTOSS       534
#define CMD_DISGUISE       535
#define CMD_CLAIM          536 // For justice clerk holding items
#define CMD_CHARGE         537
#define CMD_LORE           538
#define CMD_POOFINSND      539
#define CMD_POOFOUTSND     540
#define CMD_SWIM           541
#define CMD_NORTHWEST      542
#define CMD_SOUTHWEST      543
#define CMD_NORTHEAST      544
#define CMD_SOUTHEAST      545
#define CMD_NW             546
#define CMD_SW             547
#define CMD_NE             548
#define CMD_SE             549
#define CMD_WILL           550
#define CMD_CONDITION      551
#define CMD_NECKBITE       552
#define CMD_FORAGE         553
#define CMD_CONSTRUCT      554
#define CMD_SACK           555
#define CMD_CLIMB          556
#define CMD_MAKE           557
#define CMD_THROAT_CRUSH   558
#define CMD_TEST_DESC      559
#define CMD_TARGET         560
#define CMD_SUICIDE        561
#define CMD_ECHOG          562
#define CMD_ECHOE          563
#define CMD_ASCLIST        564 // Association list
#define CMD_WHOIS          565
#define CMD_FREE           566 // Keyword Only
#define CMD_ECHOT          567
#define CMD_ROUNDKICK      568
#define CMD_PLEASANT       569 // Inflict pleasantry
#define CMD_HAMSTRING      570
#define CMD_EMPTY          571 // Not an empty slot: "empty <container1> <container2>" moves contents from 1 into 2.
#define CMD_ARENA          572
#define CMD_ARTIFACTS      573
#define CMD_INVITE         574
#define CMD_UNINVITE       575
#define CMD_COMBINATION    576 // Combo attack, monk
#define CMD_WIZCONNECT     577
#define CMD_REPLY          578
#define CMD_DEATHOBJ       579
#define CMD_RWC            580 // Racewar chat (disabled)
#define CMD_AFFECT_PURGE   581
#define CMD_TUPOR          582
#define CMD_ECHOU          583
#define CMD_GUARD          584
#define CMD_ASSIMILATE     585
#define CMD_RANDOBJ        586
#define CMD_COMMUNE        587
#define CMD_TERRAIN        588
#define CMD_OMG            589
#define CMD_BSLAP          590
#define CMD_HUMP           591
#define CMD_BONG           592
#define CMD_SPOON          593
#define CMD_EEK            594
#define CMD_MOVING         595
#define CMD_WIT            596
#define CMD_FLAP           597
#define CMD_SPEW           598
#define CMD_ADDICT         599
#define CMD_BANZAI         600
#define CMD_BHUG           601
#define CMD_BKISS          602
#define CMD_BLINDFOLD      603
#define CMD_BOOGIE         604
#define CMD_BREW           605
#define CMD_BULLY          606
#define CMD_BUNGEE         607
#define CMD_SOB            608
#define CMD_CURL           609
#define CMD_CHERISH        610
#define CMD_CLUE           611
#define CMD_CHASTISE       612
#define CMD_COFFEE         613
#define CMD_COWER          614
#define CMD_CARROT         615
#define CMD_DOH            616
#define CMD_EKISS          617
#define CMD_FAINT          618
#define CMD_GAWK           619
#define CMD_GHUG           620
#define CMD_HANGOVER       621
#define CMD_HCUFF          622
#define CMD_HMM            623
#define CMD_ICECUBE        624
#define CMD_MAIM           625
#define CMD_MAHVELOUS      626
#define CMD_MEOW           627
#define CMD_MMMM           628
#define CMD_MOOCH          629
#define CMD_COW            630
#define CMD_MUHAHA         631
#define CMD_MWALK          632
#define CMD_NAILS          633
#define CMD_NASTY          634
#define CMD_NI             635
#define CMD_OHNO           636
#define CMD_OINK           637
#define CMD_OOO            638
#define CMD_PECK           639
#define CMD_PING           640
#define CMD_PLOP           641
#define CMD_POTATO         642
#define CMD_EPARDON        643  // social, now 'epardon' in list of commands.  Pardon is for justice code.
#define CMD_REASSURE       644
#define CMD_OHHAPPY        645
#define CMD_GLUM           646
#define CMD_SMOOCH         647
#define CMD_SQUEAL         648
#define CMD_SQUISH         649
#define CMD_STICKUP        650
#define CMD_STRIP          651
#define CMD_SUFFER         652
#define CMD_STOKE          653
#define CMD_ESWEEP         654  // social, now 'esweep' in list of commands. (sweep is for dragon pets/dracos).
#define CMD_SWOON          655
#define CMD_TENDER         656
#define CMD_THROTTLE       657
#define CMD_TIMEOUT        658
#define CMD_TORTURE        659
#define CMD_TUMMY          660
#define CMD_TYPE           661
#define CMD_WEDGIE         662
#define CMD_WISH           663
#define CMD_WRAP           664
#define CMD_YABBA          665
#define CMD_YAHOO          666
#define CMD_YEEHAW         667
#define CMD_SLOAD          668
#define CMD_CASTRATE       669
#define CMD_BOOT           670
#define CMD_GLEE           671
#define CMD_SCOWL          672
#define CMD_MUMBLE         673
#define CMD_JEER           674
#define CMD_TANK           675
#define CMD_PRAISE         676
#define CMD_BECKON         677
#define CMD_CRACK          678
#define CMD_GRENADE        679
#define CMD_CHORTLE        680
#define CMD_WHIP           681
#define CMD_CLAW           682
#define CMD_SHADES         683
#define CMD_TAN            684
#define CMD_SUNBURN        685
#define CMD_THREATEN       686
#define CMD_TWITCH         687
#define CMD_BABBLE         688
#define CMD_WRINKLE        689
#define CMD_GUFFAW         690
#define CMD_JIG            691
#define CMD_BLAME          692
#define CMD_SHUFFLE        693
#define CMD_FROTH          694
#define CMD_HOWL           695
#define CMD_MONKEY         696
#define CMD_DISGUST        697
#define CMD_VOTE           698
#define CMD_HIRE           699
#define CMD_RELOADHELP     700
#define CMD_TESTCOLOR      701
#define CMD_MULTICLASS     702
#define CMD_RESETARTI      703 // Resets timer on an arti.
#define CMD_SPECIALIZE     704
#define CMD_RESETSPEC      705
#define CMD_NCHAT          706
#define CMD_WARCRY         707
#define CMD_TACKLE         708
#define CMD_DISAPPEAR      709
#define CMD_HARDCORE       710
#define CMD_CHEAT          711
#define CMD_CHEATER        712 // Lists punished ppl
#define CMD_PUNISH         713
#define CMD_CRAFT          714
#define CMD_MIX            715
#define CMD_FORGE          716
#define CMD_THROWPOTION    717
#define CMD_PROJECTS       718 // Lists current projects defined in PROJECTS_FILE
#define CMD_RAGE           719
#define CMD_MAUL           720
#define CMD_RAMPAGE        721
#define CMD_LOTUS          722
#define CMD_TRUE_STRIKE    723
#define CMD_CHI            724
#define CMD_SHIELDPUNCH    725
#define CMD_SWEEPING_THRUST 726
#define CMD_INFURIATE      727
#define CMD_ENCRUST        728
#define CMD_GMOTD          729
#define CMD_FIX            730
#define CMD_TRAMPLE        731
#define CMD_WAIL           732 // Same as play (for Bards).
#define CMD_STATISTIC      733
#define CMD_HOME           734
#define CMD_WHIRLWIND      735
#define CMD_EPIC           736
#define CMD_THRUST         737
#define CMD_ENCHANT        738
#define CMD_UNTHRUST       739
#define CMD_RUSH           740
#define CMD_PROPERTIES     741
#define CMD_FLANK          742
#define CMD_GAZE           743
#define CMD_QUEST          744
#define CMD_RELIC          745 // Displays current Relic points for each racewar side.
#define CMD_SCENT          746 // Blood scent
#define CMD_CALL           747 // Call of the grave.
#define CMD_OK             748 // Puts online player <arg1> into ok file with message <arg rest>
#define CMD_SNEAKY_STRIKE  749
#define CMD_MUG            750
#define CMD_DEFEND         751 // Unused command.
#define CMD_EXHUME         752
#define CMD_MORE           753
#define CMD_FADE           754
#define CMD_SHRIEK         755
#define CMD_REMORT         756
#define CMD_RECALL         757
#define CMD_RAID           758 // Is raidable check
#define CMD_MINE           759
#define CMD_FISH           760
#define CMD_SPELLBIND      761
#define CMD_ASCEND         762
#define CMD_SMELT          763
#define CMD_HONE           764
#define CMD_PARLAY         765
#define CMD_NEWBIE         766
#define CMD_MAKE_GUIDE     767
#define CMD_SPELLWEAVE     768
#define CMD_DESCEND        769
#define CMD_SQL            770
#define CMD_MAKEEXIT       771
#define CMD_ROAR_OF_HEROES 772
#define CMD_NEXUS          773
#define CMD_TEST           774
#define CMD_TRANQUILIZE    775
#define CMD_TRAIN          776 // Used with nexus stones.
#define CMD_SLIP           777 // Rogue skill
#define CMD_HEADLOCK       778
#define CMD_LEGLOCK        779
#define CMD_GROUNDSLAM     780
#define CMD_INFUSE         781 // Recharges Staff/Wand
#define CMD_BUILD          782 // Builds an outpost mob? Arg: level
#define CMD_PRESTIGE       783
#define CMD_ALLIANCE       784
#define CMD_ACCUSE         785
#define CMD_DESTROY        786 // 'destroy' -> do_hit
#define CMD_SMITE          787 // Holy smite for Avengers.
#define CMD_STORAGE        788 // Create a storage container (persists through reboots/crashes/etc).
#define CMD_GATHER         789 // Gather arrows.
#define CMD_NAFK           790 // Unused command.
#define CMD_GRIMACE        791
#define CMD_NEWBSU         792 // Newbie buff <arg>
#define CMD_GIVEPET        793
#define CMD_OUTPOST        794
#define CMD_OFFENSIVE      795 // Offensive toggle
#define CMD_PERUSE         796 // Get stats on item in shop
#define CMD_HARVEST        797 // Harvest a tree for resources.
#define CMD_BATTLERAGER    798 // 'battlerager confirm' for a Dwarf to become a Berserker.
#define CMD_PETITION_BLOCK 799
#define CMD_AREA           800
#define CMD_WHITELIST      801
#define CMD_EPICRESET      802
#define CMD_FOCUS          803 // Assimilate for ShadowBeast
#define CMD_BOON           804
#define CMD_CTF	           805 // Capture the flag
#define CMD_TETHER         806 // Tether, for Cabalists (abandoned)
#define CMD_QUESTWHERE     807 // Searches for items received from quests
#define CMD_NEWBSA         808 // Newbie buff all.
#define CMD_SALVAGE        809
#define CMD_RESTRAIN       810 // Avenger & AP skill
#define CMD_BARRAGE        811 // Ranger Blade Barrage skill
#define CMD_BLADE          812 // Same as CMD_BARRAGE
#define CMD_CONSUME        813 // AP/Violator command to consume flesh.
#define CMD_RIFF           814 // Bard command
#define CMD_LEADERBOARD    815
#define CMD_SOULBIND       816
#define CMD_ACHIEVEMENTS   817
#define CMD_SALVATION      818 // Command gained from May I Heals You? Achievement
#define CMD_REFINE         819 // Refine salvaged materials
#define CMD_DREADNAUGHT    820 // Warrior/Guardian skill
#define CMD_DICE           821
#define CMD_SHADOWSTEP     822 // Rogue/Thief skill
#define CMD_GARROTE        823 // Rogue/Assassin skill
#define CMD_CONJURE        824 // Summoner command
#define CMD_DISMISS        825 // Dismisses pets.
#define CMD_ENHANCE        826 // Take two items, some plat, and make a new one.
#define CMD_ADD            827 // Add command to add resrouces to a town.
#define CMD_DEPLOY         828 // Deploy command to deploy troops in a town.
#define CMD_ADDICTED_BLOOD 829 // 'blood' command: shows Addicted to Blood/Bloodlust info
#define CMD_DEFOREST       830 // Blighter command
#define CMD_BEEP           831 // Make <arg>'s computer beep
#define CMD_DEATHS_DOOR    832 // gellz all base stats 100 achievement
#define CMD_OFFLINEMSG     833 // Send an offline message to <char>, displayed the next time they log in.
#define CMD_INSTACAST      834 // Instantly cast a spell (gods only).
#define CMD_SURNAME        835 // Set yer surname to an available option.
#define CMD_NAMEDREPORT    836 // show named equipment spells by zone

/* The CMD_s below are not real commands, they are used in item special procedures to identify
 * when wearer gets hit in melee or nuked and when an item is poofing on ground.
 */

#define CMD_MELEE_HIT     1000
#define CMD_SET_PERIODIC -10
#define CMD_PERIODIC      0
#define CMD_DEATH        -1
#define CMD_TOROOM       -2    // Checks NPC procs when someone enters a room.
#define CMD_LOOKOUT      -5
#define CMD_GOTHIT       -100
#define CMD_GOTNUKED     -101
#define CMD_MOB_COMBAT   -102
#define CMD_MOB_MUNDANE  -103
#define CMD_LOOKAFAR     -104
#define CMD_DECAY        -200
#define CMD_FOUND        -201
#define CMD_DISPEL       -202
#define CMD_BARB_REMOVE  -300  // CMD for barb hammer (vnum #17) to reset it's static vars (sigh).

#endif /* _SOJ_INTERP_H_ */
