
 ILABEL TOS.Q

DEBUG EQU 0
GSXB EQU 1;cookie
VERSION EQU $0060;$0063 max pour 0.99

INC EQU ADDQ #1,
DEC EQU SUBQ #1,
TAILLE_BUFFER EQU 256*4;8192*4
TAILLE_BUFFER_SDL EQU 1024*4
TAILLE_SOUND_HEADER EQU 64
NB_BUFFERS EQU 5
MAX_TAB EQU 8

LTATTEN EQU 0
RTATTEN EQU 1
LTGAIN EQU 2
RTGAIN EQU 3
ADDERIN EQU 4
ADCINPUT EQU 5
SETPRESCALE EQU 6
SETSMPFREQ EQU 7

;>>> SDL <<<

SDL_MIX_MAXVOLUME EQU 128

;Audio format flags
AUDIO_U8 EQU $0008                     ;Unsigned 8-bit samples
AUDIO_S8 EQU $8008                     ;Signed 8-bit samples
AUDIO_U16LSB EQU $0010                 ;Unsigned 16-bit sample Intel
AUDIO_S16LSB EQU $8010                 ;Signed 16-bit samples Intel
AUDIO_U16MSB EQU $1010                 ;Unsigned 16-bit sample Motorola
AUDIO_S16MSN EQU $9010                 ;Signed 16-bit samples Motorola
                   
;current audio state
SDL_AUDIO_STOPPED EQU 0
SDL_AUDIO_PLAYING EQU 1
SDL_AUDIO_PAUSED EQU 2

;Aranym audio functions 
OpenAudio EQU 1
CloseAudio EQU 2
PauseAudio EQU 3
AudioStatus EQU 4
AudioVolume EQU 5
LockAudio EQU 6
UnlockAudio EQU 7
GetAudioFreq EQU 8
GetAudioLen EQU 9

;>>> MacOS <<<

;flags gestalt
gestaltStereoCapability EQU 0
gestaltStereoMixing EQU 1
gestaltSoundIOMgrPresent EQU 3
gestaltBuiltInSoundInput EQU 4
gestaltHasSoundInputDevice EQU 5
gestaltPlayAndRecord EQU 6
gestalt16BitSoundIO EQU 7
gestaltStereoInput EQU 8
gestaltLineLevelInput EQU 9
gestaltSndPlayDoubleBuffer EQU 10
gestaltMultiChannels EQU 11
gestalt16BitAudioSupport EQU 12

;Get&Set Sound Information Selectors
siActiveChannels EQU 'chac'            ;active channels
siActiveLevels EQU 'lmac'              ;active meter levels
siAGCOnOff EQU 'agc '                  ;automatic gain control state
siAsync EQU 'asyn'                     ;asynchronous capability
siAVDisplayBehavior EQU 'avdb'
siChannelAvailable EQU 'chav'          ;number of channels available
siCompressionAvailable EQU 'cmav'      ;compression types available
siCompressionChannels EQU 'cpct'       ;compressor's number of channels
siCompressionFactor EQU 'cmfa'         ;current compression factor
siCompressionHeader EQU 'cmhd'         ;return compression header
siCompressionNames EQU 'cnam'          ;compression type names available
siCompressionParams EQU 'evaw'         ;compression parameters
siCompressionSampleRate EQU 'cprt'     ;compressor's sample rate
siCompressionType EQU 'comp'           ;current compression type
siContinuous EQU 'cont'                ;continous recording
siDecompressionParams EQU 'wave'       ;decompression parameters
siDeviceBufferInfo EQU 'dbin'          ;size of interrupt buffer
siDeviceConnected EQU 'dcon'           ;input device connection status
siDeviceIcon EQU 'icon'                ;input device icon
siDeviceName EQU 'name'                ;input device name
siEQSpectrumBands EQU 'eqsb'           ;number of spectrum bands
siEQSpectrumLevels EQU 'eqlv'          ;gets spectum meter levels
siEQSpectrumOnOff EQU 'eqlo'           ;turn on/off spectum meter levels
siEQSpectrumResolution EQU 'eqrs'      ;set the resolution of the FFT, 0 = low res (<=16 bands), 1 = high res (16-64 bands)
siEQToneControlGain EQU 'eqtg'         ;set the bass and treble gain
siEQToneControlOnOff EQU 'eqtc'        ;turn on equalizer attenuation
siHardwareBalance EQU 'hbal'
siHardwareBalanceSteps EQU 'hbls'
siHardwareBass EQU 'hbas'
siHardwareBassSteps EQU 'hbst'
siHardwareBusy EQU 'hwbs'              ;sound hardware is in use
siHardwareFormat EQU 'hwfm'            ;get hardware format
siHardwareMute EQU 'hmut'              ;mute state of all hardware
siHardwareMuteNoPrefs EQU 'hmnp'       ;mute state of all hardware, but don't store in prefs 
siHardwareTreble EQU 'htrb'
siHardwareTrebleSteps EQU 'hwts'
siHardwareVolume EQU 'hvol'            ;volume level of all hardware
siHardwareVolumeSteps EQU 'hstp'       ;number of volume steps for hardware
siHeadphoneMute EQU 'pmut'             ;mute state of headphones
siHeadphoneVolume EQU 'pvol'           ;volume level of headphones
siHeadphoneVolumeSteps EQU 'hdst'      ;number of volume steps for headphones
siInputAvailable EQU 'inav'            ;input sources available
siInputGain EQU 'gain'                 ;input gain
siInputSource EQU 'sour'               ;input source selector
siInputSourceNames EQU 'snam'          ;input source names
siLevelMeterOnOff EQU 'lmet'           ;level meter state
siModemGain EQU 'mgai'                 ;modem input gain
siMonitorAvailable EQU 'mnav'
siMonitorSource EQU 'mons'
siNumberChannels EQU 'chan'            ;current number of channels
siOptionsDialog EQU 'optd'             ;display options dialog
siOSTypeInputSource EQU 'inpt'         ;input source by OSType
siOSTypeInputAvailable EQU 'inav'      ;list of available input source OSTypes
siOutputDeviceName EQU 'onam'          ;output device name
siPlayThruOnOff EQU 'plth'             ;playthrough state
siPostMixerSoundComponent EQU 'psmx'   ;install post-mixer effect
siPreMixerSoundComponent EQU 'prmx'    ;install pre-mixer effect
siQuality EQU 'qual'                   ;quality setting
siRateMultiplier EQU 'rmul'            ;throttle rate setting
siRecordingQuality EQU 'qual'          ;recording quality
siSampleRate EQU 'srat'                ;current sample rate
siSampleRateAvailable EQU 'srav'       ;sample rates available
siSampleSize EQU 'ssiz'                ;current sample size
siSampleSizeAvailable EQU 'ssav'       ;sample sizes available
siSetupCDAudio EQU 'sucd'              ;setup sound hardware for CD audio
siSetupModemAudio EQU 'sumd'           ;setup sound hardware for modem audio
siSlopeAndIntercept EQU 'flap'         ;floating point variables for conversion
siSoundClock EQU 'sclk'
siUseThisSoundClock EQU 'sclc'         ;sdev uses this to tell the mixer to use his sound clock
siSpeakerMute EQU 'smut'               ;mute state of all built-in speaker
siSpeakerVolume EQU 'svol'             ;volume level of built-in speaker
siSSpCPULoadLimit EQU '3dll'
siSSpLocalization EQU '3dif'
siSSpSpeakerSetup EQU '3dst'
siStereoInputGain EQU 'sgai'           ;stereo input gain
siSubwooferMute EQU 'bmut'             ;mute state of sub-woofer
siTerminalType EQU 'ttyp'              ;usb terminal type 
siTwosComplementOnOff EQU 'twos'       ;two's complement state
siVendorProduct EQU 'vpro'             ;vendor and product ID 
siVolume EQU 'volu'                    ;volume level of source
siVoxRecordInfo EQU 'voxr'             ;VOX record parameters
siVoxStopInfo EQU 'voxs'               ;VOX stop parameters
siWideStereo EQU 'wide'                ;wide stereo setting
siSupportedExtendedFlags EQU 'exfl'    ;which flags are supported in Extended sound data structures
siRateConverterRollOffSlope EQU 'rcdb' ;the roll-off slope for the rate converter's filter, in whole dB 
; as a long this value is a long whose range is from 20 (worst quality/fastest performance)
; to 90 (best quality/slowest performance)
siOutputLatency EQU 'olte'             ;latency of sound output component

;commandes de SNDDOCOMMAND ou SNDDOIMMEDIATE
NULLCMD EQU 0                          ;do nothing
QUIETCMD EQU 3                         ;stop a sound that is playing
FLUSHCMD EQU 4                         ;flush a sound channel
REINITCMD EQU 5                        ;reinitialize a sound channel
WAITCMD EQU 10                         ;suspend processing in a channel
PAUSECMD EQU 11                        ;pause processing in a channel
RESUMECMD EQU 12                       ;resume processing in a channel
CALLBACKCMD EQU 13                     ;execute a callback procedure
SYNCCMD EQU 14                         ;synchronize channels
AVAILABLECMD EQU 24                    ;see if initialization options are supported
VERSIONCMD EQU 25                      ;determine version
TOTALLOADCMD EQU 26                    ;report total CPU load
LOADCMD EQU 27                         ;report CPU load for a new channel
FREQDURATIONCMD EQU 40                 ;play a note for a duration
RESTCMD EQU 41                         ;rest a channel for a duration
FREQCMD EQU 42                         ;change the pitch of a sound
AMPCMD EQU 43                          ;change the amplitude of a sound
TIMBRECMD EQU 44                       ;change the timbre of a sound
GETAMPCMD EQU 45                       ;get the amplitude of a sound
VOLUMECMD EQU 46                       ;set volume
GETVOLUMECMD EQU 47                    ;get volume
WAVETABLECMD EQU 60                    ;install a wave table as a voice
SOUND_CMD EQU 80                       ;install a sampled sound as a voice
BUFFERCMD EQU 81                       ;play a sampled sound
RATECMD EQU 82                         ;set the pitch of a sampled sound
GETRATECMD EQU 85                      ;get the pitch of a sampled sound

;CONST 1 de SNDNEWCHANNEL
SQUAREWAVESYNTH EQU 1
WAVETABLESYNTH EQU 3
SAMPLEDSYNTH EQU 5

;CONST 2 de SNDNEWCHANNEL
INITCHANLEFT EQU 2
INITCHANRIGHT EQU 3
WAVEINITCHANNEL0 EQU 4
WAVEINITCHANNEL1 EQU 5
WAVEINITCHANNEL2 EQU 6
WAVEINITCHANNEL3 EQU 7
INITMONO EQU $80
INITSTEREO EQU $C0
INITMACE3 EQU $300
INITMACE6 EQU $400
INITNOINTERP EQU 4
INITNODROP EQU 8

;structure SNDCHANNEL
NEXTCHAN EQU 0;SndChannelPtr
FIRSTMOD EQU 4
CALLBACK EQU 8;SndCallBackProcPtr
USERINFO EQU 12
WAIT EQU 16
CMDINPROGRESS EQU 20;SndCommand
FLAGS EQU 24
QLENGTH EQU 26
QHEAD EQU 28
QTAIL EQU 30
QUEUE EQU 32;SndCommand [128]

;structure INPARAMPTR
_INREFNUM EQU 0;           --> reference number of a sound input device
_COUNTREC EQU 4;          <--> number of bytes of recording
_MILLISECONDS EQU 8;      <--> number of milliseconds of recording
_BUFFERLENGTH EQU 12;      --> length of the buffer beginning at bufferPtr
_BUFFERPTR EQU 16;         --> pointer to a buffer for sampled-sound data
_COMPLEXIONROUTINE EQU 20; --> pointer to a completion routine
_INTERRUPTROUTINE EQU 24;  --> pointer to an interrupt routine
_USERLONG EQU  28;         --> free for application's use
_ERRORREC EQU 32;         <--  word 1 during record, 0 if finished, < 0 if error

VECT_TIMER_A EQU $134
VECT_IO7 EQU $13C
MEMVALID EQU $420
RESVALID EQU $426
RESVECTOR EQU $42A
PROC_TYPE EQU $59E
_COOKIE EQU $5A0

;Le cookie 'McSn' (sous n'importe quel version TOS) est une garantie,
;qu'on peut jouer des sons Ö 12.5 KHz 8Bit Mono par les fonctions son 
;XBIOS Falcon. 
;Pour plus d'informations, il faut tester le cookie McSn:
;
; struct McSnCookie
; {
;    short   vers;   Version en code BCD
;    short   size;   Taille de la structure
;    
;    short   play;   PossibilitÇs de lecture
;    short   record; PossibilitÇs d'enregistrement
;    short   dsp;    Y'a-t-il un DSP?
;    short   pint;   Playing: Interrupt possible pour fin de Frame?
;    short   rint;   Recording: Interrupt possible pour fin de Frame?
;
;    long    res1;   rÇservÇ.......
;    long    res2;
;    long    res3;
;    long    res4;
; };
;
;Valeurs pour play & record:
; #define ST      0
; #define TT      1
; #define FALCON  2
;
;Valeurs pour play:
; ST     -> 12.5 Khz, 8 Bit Mono!
; TT     -> FrÇquences STE/TT, 8 Bit Stereo
; FALCON -> FrÇquences FALCON, 16 Bit Stereo
;
;Et pour record:
; ST     -> Pas de possibilitÇs d'enregistrement
; TT     -> Pas de possibilitÇs d'enregistrement
; FALCON -> FrÇquences FALCON, 16 Bit Stereo
; 
;VoilÖ Ö quoi devraient resembler les tests pour play & record: 
; if play >= FALCON then [PossibilitÇs FALCON prÇsentes]
;Pour dsp, pint & rint:
; if pint!=0 then setinterrupt marche por la lecture.
;Sur tous le Macintosh avec MagiCMac: play = FALCON,
;sans regarder quelles possibilitÇs sonores du Macintosh (mono/stereo ..),

TIMER_OUTPUT:MACRO $\1,$\2,$\3,$\4

 MOVE \1,-(SP);mode: B0:mono(0)/stÇrÇo(1)  B1:8 bits(0)/16 bits(1)
;                    B6:rÇpÇtition non(0)/oui(1)
 MOVE \2,-(SP);frÇquence
 PEA \3;adresse Çchantillons
 MOVE.L \4,-(SP);longueur
 BSR TIMER_OUTPUT_
 LEA 12(SP),SP
 ENDM

DEBUT:

 MOVE.L 4(SP),A5;DÇbut de la basepage
 MOVE.L $C(A5),D0
 ADD.L $14(A5),D0
 ADD.L $1C(A5),D0
 ADD.L #256,D0
 SETBLOCK D0,(A5)
 MOVE.L #"MgSn",D0
 BSR GET_COOKIE
 BPL.S \1;dÇja installÇ
 BSR INIT
 BMI.S \1;erreur
 KEEPPROC #0,#FIN-DEBUT+$200;programme rÇsidant
\1:IFNE 0,DEBUG
  PRINTLINE DEBUG1
  CONIN_WE
 ENDIF
 TERM
 RTS

AFF_INFO:

 MOVEM.L D0-D1/A0,-(SP)
 LINK A6,#-4
 PRINTLINE MESS1
 LEA -4(A6),A0
 CLR.L (A0)
 MOVEQ #VERSION/256,D0
 MOVEQ #2,D1
 BSR CONV_DECI
 MOVE.B #".",-2(A6)
 PRINTLINE -4(A6)
 LEA -4(A6),A0
 CLR.L (A0)
 MOVEQ #VERSION & 255,D0
 MOVEQ #2,D1
 BSR CONV_DECI_SIMPLE
 PRINTLINE -4(A6)
 PRINTLINE MESS2
 TST D_MAC
 BNE.S \1;MagiCMac
 TST D_SDL
 BEQ.S \1
 PRINTLINE MESS8 
\1:UNLK A6
 MOVEM.L (SP)+,D0-D1/A0
 RTS

AFF_NUMVERSION:

 MOVEM.L D0-D1/A0,-(SP)
 SWAP D0
 MOVE D0,D1
 MOVE D1,D0
 LSR #8,D0
 AND #$F,D0
 OR #$30,D0
 CONOUT D0
 CONOUT #"."
 MOVE D1,D0
 LSR #4,D0
 AND #$F,D0
 OR #$30,D0
 CONOUT D0
 MOVEQ #$F,D0
 AND D1,D0
 OR #$30,D0
 CONOUT D0 
 MOVEM.L (SP)+,D0-D1/A0
 RTS
 
AFF_NOM_LISTE:;index dans D0, chaåne dans A0

 MOVEM.L D0-D1/A0-A1,-(SP)
 MOVE D0,D1
 MOVE.L A0,A1
 CONOUT #13
 CONOUT #10
 CONOUT #"<"
 OR #$30,D1
 CONOUT D1
 CONOUT #">"
 CONOUT #" "
 PRINTLINE (A1)
 CONOUT #" "
 MOVEM.L (SP)+,D0-D1/A0-A1
 RTS

CONV_CAR:;MAC => Atari

 MOVE.L A0,-(SP)
 LEA TAB_CAR_MAC,A0
\2:TST.B (A0)+
  BEQ.S \1
 CMP.B (A0)+,D0
 BNE.S \2
 MOVE.B -2(A0),D0
\1:MOVE.L (SP)+,A0
 RTS

CONV_CHAINE:;chaåne dans A0

 MOVEM.L D0/D1/A0,-(SP)
 MOVE #255,D1
\1:MOVE.B (A0),D0
  BSR CONV_CAR
  MOVE.B D0,(A0)+
 DBEQ D1,\1
 MOVEM.L (SP)+,D0/D1/A0
 RTS 

INIT:

 MOVEM.L D0-D2/A0-A2/A5,-(SP)
 CLR D_MAC
 CLR D_TT
 CLR D_SDL
 MOVE.L #"_MCH",D0
 BSR GET_COOKIE
 BMI.S \38
 MOVE.L D0,A0
 MOVE (A0),D0;donnÇe cookie
 CMP #2,D0;TT
 SEQ.B D0
 EXT.W D0
 MOVE D0,D_TT
\38:MOVE.L #"PSND",D0;carte son HADES
 BSR GET_COOKIE
 SPL.B D0
 AND #1,D0;flag carte son HADES
 MOVE D0,INFOS_COOKIE+10;slot
 MOVE.L #"MgMc",D0
 BSR GET_COOKIE
 BMI \39;<> MagiCMac
 ST.B D_MAC
 MOVE.L D0,A0
 MOVE.L (A0),D0;donnÇe cookie
 BEQ \1
 MOVE.L D0,A2
 MOVE.L $10(A2),MODEMAC
 MOVE.L $14(A2),MODEATARI
 MOVE.L $18(A2),GETBASEMODE
 MOVE.L $28(A2),CALLMACCONTEXT
 MOVE.L $2C(A2),ATARIZEROPAGE
 IFNE 0,DEBUG
  PRINTLINE DEBUG3
 ENDIF
 LEA INFOS_SON_MAC,A0
 MOVE.L #"snd ",D0
 BSR GESTALT
 BEQ.S \5
 PRINTLINE ERR2;pas de son MAC
 BRA \6
\5:IFNE 0,DEBUG
  MOVEM.L D0/D1/A0,-(SP)
  LINK A6,#-6
  CLR -2(A6)
  LEA -6(A6),A0
  MOVE.L INFOS_SON_MAC,D0
  MOVEQ #5,D1
  BSR CONV_DECI
  PRINTLINE -6(A6)
  UNLK A6
  MOVEM.L (SP)+,D0/D1/A0
 ENDIF
 MOVE.L INFOS_SON_MAC,D0
 BTST #gestaltStereoCapability,D0
 BNE.S \28
 PRINTLINE ERR8;pas de lecture stÇrÇo
 BRA \6
\28:BTST #gestalt16BitSoundIO,D0
 BNE.S \29
 PRINTLINE ERR9;pas de sons 16 bits
 BRA \6
\29:IFNE 0,DEBUG
  PRINTLINE DEBUG4
 ENDIF
 LEA CHANNEL,A0
 MOVE #128,QTAIL(A0)
 PEA (A0)
 LEA (SP),A0
 MOVE.L A0,SNDCHANNELPTR2
 MOVE.L #INITSTEREO,INITSND
 LEA CALLBACK16,A0 
 MOVE.L A0,SNDCALLBACKPROCPTR
 LEA SNDNEWCHANNEL,A0
 BSR CALLMACFUNCTION
 ADDQ #4,SP
 BEQ \4
 PRINTLINE ERR1;canal ne peut pas etre ouvert
\6:CONIN_WE
 CONOUT #13
 CONOUT #10
 CONOUT #10
 BRA \1
\39:SUPEXEC TEST_SDL_ARANYM
 CLR D_OPEN_SDL
\4:MOVE.L #"MgSn",D0
 BSR INIT_COOKIE
 BPL.S \3
\1:MOVEQ #-1,D0;erreur
 BRA \2
\3:MOVE.L D0,A0
 LEA INFOS_COOKIE,A5
 MOVE.L A5,(A0)
 MOVE.L #"McSn",D0
 BSR INIT_COOKIE
 BMI.S \9;erreur
 MOVE.L D0,A0
 LEA STRUCT_MCSN,A1
 MOVE.L A1,(A0)
\9:IFNE 0,DEBUG
  PRINTLINE DEBUG5
 ENDIF
 IFNE 0,GSXB
  MOVE.L #"GSXB",D0
  BSR GET_COOKIE
  BPL.S \12
  MOVE.L #"GSXB",D0
  BSR INIT_COOKIE
\12:
 ENDIF
 MOVEQ #6+32,D1;Çtendu, 16-bit DMA & 8-bit DMA
 MOVE.L #"_SND",D0
 BSR GET_COOKIE
 BMI.S \34;pas trouvÇ
 MOVE.L D0,A0
 MOVEQ #1,D0
 AND.L (A0),D0;conserve bit PSG
 OR.L D0,D1
 BRA.S \35
\34:MOVE.L #"_SND",D0
 BSR INIT_COOKIE
 BMI.S \11;erreur
\10:MOVE.L D0,A0
; Bit 0: PSG soundchip
; Bit 1: 8-bit DMA
; Bit 2: 16-bit DMA
; Bit 3: DSP
; Bit 4: Matrix
; Bit 5: Çtendu
\35:MOVE.L D1,(A0)
 IFNE 0,DEBUG
  PRINTLINE DEBUG2
 ENDIF 
\11:TST D_MAC
 BEQ \36;<> MagiCMac
 LEA DET_FIN_SON,A0
 MOVE.L $34(A2),-4(A0);MACAPPSWITCH
 MOVE.L A0,$34(A2)
 LEA TAB_BUFFERS,A2
 MOVE #NB_BUFFERS-1,D2
\7:LEA TAILLE_SOUND_HEADER(A2),A0
  MOVE.L A0,(A2)+;buffer samplePtr
  MOVE.L #2,(A2)+;numChannels
  CLR.L (A2)+;frÇquence * 65536
  CLR.L (A2)+;loopStart
  CLR.L (A2)+;loopEnd
  MOVE #$FF00,(A2)+;encode (extSH), baseFrequency
  MOVE.L #TAILLE_BUFFER/4,(A2)+;numFrames
  CLR.L (A2)+;AIFFSampleRate sur 80 bits
  CLR.L (A2)+
  CLR (A2)+
  CLR.L (A2)+;markerChunk
  CLR.L (A2)+;instrumentChunks
  CLR.L (A2)+;AESRecording
  MOVE #16,(A2)+;sampleSize
  CLR (A2)+;futurUse1
  CLR.L (A2)+;futurUse2
  CLR.L (A2)+;futurUse3
  CLR.L (A2)+;futurUse4
  ADD.L #TAILLE_BUFFER,A2
 DBRA D2,\7
 LEA TAILLE_SOUND_HEADER(A2),A0
 MOVE.L A0,(A2)+;buffer samplePtr
 MOVE.L #2,(A2)+;numChannels
 CLR.L (A2)+;frÇquence * 65536
 CLR.L (A2)+;loopStart
 CLR.L (A2)+;loopEnd
 MOVE #$FF00,(A2)+;encode (extSH), baseFrequency
 MOVE.L #TAILLE_BUFFER/4,(A2)+;numFrames
 CLR.L (A2)+;AIFFSampleRate sur 80 bits
 CLR.L (A2)+
 CLR (A2)+
 CLR.L (A2)+;markerChunk
 CLR.L (A2)+;instrumentChunks
 CLR.L (A2)+;AESRecording
 MOVE #16,(A2)+;sampleSize
 CLR (A2)+;futurUse1
 CLR.L (A2)+;futurUse2
 CLR.L (A2)+;futurUse3
 CLR.L (A2)+;futurUse4
 MOVE.L #(TAILLE_BUFFER/4)-1,D1;numFrames -1
\8:CLR.L (A2)+;init buffer arràt son
 SUBQ.L #1,D1
 BPL.S \8
 BSR RAZ_BUFFERS
\36:BSR INIT_DET
 BSR AFF_INFO
 TST D_MAC
 BEQ \16;<> MagiCMac
 IFNE 0,DEBUG
  MOVEM.L D0/A0,-(SP)
  PRINTLINE DEBUG50
  MOVEM.L (SP)+,D0/A0
 ENDIF 
 PRINTLINE MESS3
 LEA SNDSOUNDMANAGERVERSION,A0
 BSR CALLMACFUNCTION
 BSR AFF_NUMVERSION
 CLR NB_INDEV
 MOVE.L INFOS_SON_MAC,D0
; BTST #gestaltSoundIOMgrPresent,D0
 BTST #gestaltHasSoundInputDevice,D0
 BNE.S \26
 PRINTLINE ERR7;no input device
 BRA \16
\26:BTST #gestaltStereoInput,D0
 BNE.S \27
 PRINTLINE ERR10;pas d'enregistrement stÇrÇo
 BRA \16
\27:IFNE 0,DEBUG
  MOVEM.L D0/A0,-(SP)
  PRINTLINE DEBUG126
  MOVEM.L (SP)+,D0/A0
 ENDIF 
 PRINTLINE MESS4
 LEA SPBVERSION,A0
 BSR CALLMACFUNCTION
 BSR AFF_NUMVERSION  
 PRINTLINE MESS5
 MOVE #1,COUNT
 MOVEQ #MAX_TAB-1,D1
\13:IFNE 0,DEBUG
   MOVEM.L D0/A0,-(SP)
   PRINTLINE DEBUG124
   MOVEM.L (SP)+,D0/A0
  ENDIF
  LEA SPBGETINDEXEDDEVICE,A0
  BSR CALLMACFUNCTION
  BNE \14
  LEA DEVICENAME,A1
  MOVEQ #0,D0
  MOVE.B (A1)+,D0;format pascal
  CLR.B (A1,D0);fin chaåne
  MOVE.L A1,A0
  BSR CONV_CHAINE
  MOVE COUNT,D0
  BSR AFF_NOM_LISTE
  INC COUNT
 DBRA D1,\13
\14:MOVE COUNT,D0
 DEC D0
 MOVE D0,NB_INDEV
 BEQ.S \17 
 IFNE 0,DEBUG
  MOVEM.L D0/A0,-(SP)
  PRINTLINE DEBUG125
  MOVEM.L (SP)+,D0/A0
 ENDIF
 LEA SPBOPENDEVICE,A0
 BSR CALLMACFUNCTION
 BEQ \15
\18:CLR NB_INDEV
\17:PRINTLINE ERR3;erreur ouverture
 BRA \16
\22:PRINTLINE ERR4;pas de source
 BRA.S \25 
\23:PRINTLINE ERR5;pas de stÇrÇo
 BRA.S \25
\24:PRINTLINE ERR6;pas de 16 bits
\25:IFNE 0,DEBUG
  MOVEM.L D0/A0,-(SP)
  PRINTLINE DEBUG127
  MOVEM.L (SP)+,D0/A0
 ENDIF 
 LEA SPBCLOSEDEVICE,A0
 BSR CALLMACFUNCTION
 BRA \18
\15:IFNE 0,DEBUG
  MOVEM.L D0/A0,-(SP)
  PRINTLINE DEBUG120
  PRINTLINE DEBUG160
  MOVEM.L (SP)+,D0/A0
 ENDIF
 MOVE.L #siDeviceName,INFOTYPE
 LEA SPBGETDEVICEINFO,A0
 BSR CALLMACFUNCTION
 BNE \18;erreur
 PRINTLINE MESS6
 LEA INFODATA,A0
 MOVEQ #0,D0
 MOVE.B (A0)+,D0;format pascal
 CLR.B (A0,D0);fin chaåne
 BSR CONV_CHAINE
 PRINTLINE (A0)
 IFNE 0,DEBUG
  MOVEM.L D0/A0,-(SP)
  PRINTLINE DEBUG150
  MOVEM.L (SP)+,D0/A0
 ENDIF
 MOVE.L #siInputSource,INFOTYPE
 LEA SPBGETDEVICEINFO,A0
 BSR CALLMACFUNCTION
 BNE \22;erreur
 MOVE INFODATA,D0
 MOVE D0,INSOURCE
 MOVE D0,4(A5)
 IFNE 0,DEBUG
  MOVEM.L D0/A0,-(SP)
  PRINTLINE DEBUG159
  MOVEM.L (SP)+,D0/A0
 ENDIF
 CLR NB_SOURCES
 LEA TAB_SOURCENAME,A2
 CLR.B (A2);fin tableau
 MOVE.L #siInputSourceNames,INFOTYPE
 LEA SPBGETDEVICEINFO,A0
 BSR CALLMACFUNCTION
 BNE \30;erreur
 LEA INFODATA,A0
 MOVE.L (A0),A0;Handle
 MOVE.L (A0),A1
 MOVE (A1)+,D1
 MOVE D1,NB_SOURCES
 MOVE D1,2(A5)
 DEC D1
 BMI \30
 PRINTLINE MESS7
 MOVEQ #1,D3
 CMP #MAX_TAB,D1
 BCS.S \31
 MOVEQ #MAX_TAB,D1
\31:MOVEQ #0,D2
  MOVE.B (A1)+,D2;format pascal
  MOVE.L A2,A0
  MOVE D2,D0
  DEC D0
  BMI.S \32
  MOVE.L A2,A0
\33:MOVE.B (A1)+,(A2)+
  DBRA D0,\33
  CLR.B (A2)+
  BSR CONV_CHAINE
  MOVE D3,D0
  BSR AFF_NOM_LISTE
  CMP INSOURCE,D3
  BNE.S \32
  CONOUT #"<"
  CONOUT #"-"
\32:INC D3
 DBRA D1,\31
 CLR.B (A2);fin tableau
\30:IFNE 0,DEBUG
  MOVEM.L D0/A0,-(SP)
  PRINTLINE DEBUG151
  MOVEM.L (SP)+,D0/A0
 ENDIF
 MOVE.L #siAsync,INFOTYPE
 LEA SPBGETDEVICEINFO,A0
 BSR CALLMACFUNCTION
 BNE \25;erreur
 TST INFODATA
 BEQ \25;<> asynchronous
 IFNE 0,DEBUG
  MOVEM.L D0/A0,-(SP)
  PRINTLINE DEBUG152
  MOVEM.L (SP)+,D0/A0
 ENDIF
 MOVE.L #siNumberChannels,INFOTYPE
 LEA SPBGETDEVICEINFO,A0
 BSR CALLMACFUNCTION
 BNE \23;erreur
 CMP #2,INFODATA
 BCS \23 
 IFNE 0,DEBUG
  MOVEM.L D0/A0,-(SP)
  PRINTLINE DEBUG153
  MOVEM.L (SP)+,D0/A0
 ENDIF
 MOVE.L #siSampleSize,INFOTYPE
 LEA SPBGETDEVICEINFO,A0
 BSR CALLMACFUNCTION
 BNE \24;erreur 
 CMP #16,INFODATA
 BCS \24
 IFNE 0,DEBUG
  MOVEM.L D0/A0,-(SP)
  PRINTLINE DEBUG154
  MOVEM.L (SP)+,D0/A0
 ENDIF
 MOVE.L #siSampleRateAvailable,INFOTYPE
 LEA SPBGETDEVICEINFO,A0
 BSR CALLMACFUNCTION 
 BNE \25
 LEA INFODATA,A0
 MOVE (A0)+,D0;count
 CMP #16,D0
 BCS.S \19
 MOVEQ #16,D0
\19:MOVE D0,NB_FREQ_REC
 BNE.S \20;liste frÇquences enregistrement supportÇes
 MOVEQ #2,D0;mini et maxi frÇquences enregistrement supportÇs
\20:MOVE.L (A0),A0;Handle
 MOVE.L (A0),A0
 LEA TAB_FREQ_REC,A1
 DEC D0
\21:MOVE.L (A0)+,(A1)+
 DBRA D0,\21        
 IFNE 0,DEBUG
  MOVEM.L D0/A0,-(SP)
  PRINTLINE DEBUG121
  PRINTLINE DEBUG152
  MOVEM.L (SP)+,D0/A0
 ENDIF
 MOVE #2,INFODATA;2 voies
 MOVE.L #siNumberChannels,INFOTYPE
 LEA SPBSETDEVICEINFO,A0
 BSR CALLMACFUNCTION
 BNE \25;erreur
 IFNE 0,DEBUG
  MOVEM.L D0/A0,-(SP)
  PRINTLINE DEBUG153
  MOVEM.L (SP)+,D0/A0
 ENDIF
 MOVE #16,INFODATA;16 bits
 MOVE.L #siSampleSize,INFOTYPE
 LEA SPBSETDEVICEINFO,A0
 BSR CALLMACFUNCTION
 BNE \25;erreur
 IFNE 0,DEBUG
  MOVEM.L D0/A0,-(SP)
  PRINTLINE DEBUG155
  MOVEM.L (SP)+,D0/A0
 ENDIF
 MOVE.L #"NONE",INFODATA 
 MOVE.L #siCompressionType,INFOTYPE
 LEA SPBSETDEVICEINFO,A0
 BSR CALLMACFUNCTION
 BNE \25;erreur
 IFNE 0,DEBUG
  MOVEM.L D0/A0,-(SP)
  PRINTLINE DEBUG156
  MOVEM.L (SP)+,D0/A0
 ENDIF
 MOVE #1,INFODATA;Çchantillons signÇs
 MOVE.L #siTwosComplementOnOff,INFOTYPE
 LEA SPBSETDEVICEINFO,A0
 BSR CALLMACFUNCTION
 BNE \25;erreur
 IFNE 0,DEBUG
  MOVEM.L D0/A0,-(SP)
  PRINTLINE DEBUG157
  MOVEM.L (SP)+,D0/A0
 ENDIF
 MOVE #7,INFODATA
 MOVE.L #siPlayThruOnOff,INFOTYPE
 LEA SPBSETDEVICEINFO,A0
 BSR CALLMACFUNCTION
 BNE \25;erreur 
 SOUNDCMD #-1,#LTGAIN;init GAINS
 SOUNDCMD #-1,#RTGAIN
\16:TST NB_INDEV
 BNE.S \37
 CLR STRUCT_MCSN+6;record ST (impossible)
\37:CONOUT #13
 CONOUT #10
 CONOUT #10
 MOVEQ #0,D0
\2:MOVEM.L (SP)+,D0-D2/A0-A2/A5
 RTS

INIT_DET:

 MOVEM.L D0-D2/A0-A2,-(SP)
 CLR NB_INDEV
 CLR FLAG_ARRET
 MOVEQ #2,D0
 MOVE D0,NB_VOIES
 MOVE D0,OCT_ECH;8 bits stÇrÇo
 CLR PRESCALE
 CLR PRESCALE_STE
 CLR HORLOGE
 CLR HORLOGE2
 CLR NB_TRACKS
 CLR NB_TRACKS_REC
 CLR TRACK
 CLR.L INTH_ADDR;routine appelÇe en fin lecture buffer DMA
 CLR.L INTH_ADDR_REC;routine appelÇe en fin enregistrement buffer DMA
 CLR CAUSE_INTER;setinterrupt
 CLR D_TIMER_A
 CLR D_IO7
 CLR COUNT_TIMER_A
 CLR D_INIT_TIMER_A
 SETEXEC DET_BIOS,#45;TRAP #13
 MOVE.L D0,DET_BIOS-4
 SETEXEC DET_XBIOS,#46;TRAP #14
 MOVE.L D0,DET_XBIOS-4
 MOVEM.L (SP)+,D0-D2/A0-A2
 RTS

 DC.L "XBRA"
 DC.L "MgSn"
 DC.L 0

CALLBACK16_SDL:

 MOVEM.L D0-A6,-(SP)
 MOVE D_INTEL_SDL,-(SP)
 TST D_OPEN_SDL
 BEQ \1
 BTST #0,STATUS+1
 BEQ.S \78;arràt lecture
 BSR SDL_AUDIOSTATUS
 CMP #SDL_AUDIO_PLAYING,D0
 BEQ.S \71
\78:AND #$FFFC,STATUS;arràt lecture DMA
 BSR SDL_CLOSEAUDIO
 CLR D_OPEN_SDL
 BRA \1
\71:BTST #0,STATUS+1
 BEQ \1;arràt lecture
 BSR SDL_LOCKAUDIO
 BSR SDL_GETAUDIOLEN
 MOVE.L D0,D3;taille buffer cible
 LEA TAB_BUFFERS,A2;cible dÇbut
 MOVE.L A2,A1;adresse Çchantillons cible
 ADD.L D3,A1;adresse de fin cible
 MOVE.L ADR,A0;adresse source
 MOVE.L ADR_FIN,A6;fin source => changement de buffer D2D
 MOVE.L FREQUENCE,D6;frÇquence source
 MOVE.L FREQ_LECT,D7;frÇquence cible
 CMP.L D6,D7
 BEQ \8;frÇquences identiques => pas de rÇÇchantillonnage
 CMP #2,NB_VOIES
 BEQ \45;stÇrÇo
 CMP #2,OCT_ECH
 BEQ \58;16 bits mono
;8 bits mono
 ADD TRACK,A0
 MOVE NB_TRACKS,A5
 CMP.L D6,D7
 BCS \35;diminution de la longueur
;cas ou frÇquence cible > frÇquence source
;   => augmentation de la longueur
 MOVEQ #0,D0
 MOVE.L D6,D1;frÇquence source
 LSR.L #1,D1
 ROXR.L #1,D0
 MOVE.L D7,D2;rapport = frÇquence source * 2^31 / frÇquence cible
 JSR DIV64_32S
 ADD.L D0,D0;*2 => 2^31 sinon pb signe
 MOVEQ #0,D1
 MOVE NB_TRACKS,D1;0-3
 INC D1
 JSR MUL32_32
 MOVE.L D0,D1;pas
 MOVEQ #0,D2
 MOVEQ #0,D3
\33:MOVE.L A0,A3;adresse dÇbut source
  ADD.L D3,A3
  MOVE.B (A3)+,D6;Çchantillon 8 bits x(n)
  ASL #8,D6
  MOVE.L D2,D0
  SWAP D0
  LSR #1,D0;1-32767
  BEQ.S \32
  ADD A5,A3
  MOVE.B (A3)+,D7;Çchantillon 8 bits mono x(n+1)
  ASL #8,D7
  MULS D0,D7
  NEG D0
  BCLR #15,D0;1-32767
  MULS D0,D6
  ADD.L D7,D6
  ADD.L D6,D6
  SWAP D6
\32:CMP.L A6,A3
  BCC.S \31;fin buffer
\34:TST (SP)
  BPL.S \59
  ROR #8,D6;format Intel
\59:MOVE D6,(A2)+;lecture en 16 bits stÇrÇo
  MOVE D6,(A2)+
  MOVEQ #0,D0
  ADD.L D1,D2;pas
  ADDX.L D0,D3
 CMP.L A1,A2
 BCS.S \33
 BRA \6
\31:ADD A5,A3
 MOVE.L A3,ADR
 BTST #1,STATUS+1
 BEQ \5;lecture sans rÇpÇtition
 BSR SWAP_ADR
 MOVE.L ADR_DEBUT,A0
 ADD TRACK,A0
 MOVE.L ADR_FIN,A6
 BRA.S \34
;cas ou frÇquence cible < frÇquence source
;   => diminution de la longueur
\35:MOVEQ #0,D0
 MOVE.L D6,D1;frÇquence source
 MOVE.L D7,D2
 ASL.L #8,D2;rapport = frÇquence source * 2^31 / frÇquence cible * 2^8
 JSR DIV64_32S
 MOVE.L D0,D1
 ASL.L #8,D0
 ROL.L #8,D1
 AND.L #$FF,D1;poids fort
 JSR MUL64_TRACKS
 MOVE.L D0,A4;poids faible
 MOVEQ #0,D2
 MOVEQ #0,D3
\37:MOVE.L A0,A3;adresse dÇbut source
  ADD.L D3,A3
  MOVE.B (A3)+,D6;Çchantillon 8 bits mono x(n)
  ASL #8,D6
  MOVE.L D2,D0
  SWAP D0
  LSR #1,D0;1-32767
  BEQ.S \38
  ADD A5,A3
  MOVE.B (A3)+,D7;Çchantillon 8 bits mono x(n+1)
  ASL #8,D7
  MULS D0,D7
  NEG D0
  BCLR #15,D0;1-32767
  MULS D0,D6
  ADD.L D7,D6
  ADD.L D6,D6
  SWAP D6
\38:CMP.L A6,A3
  BCC.S \39;fin buffer
\36:TST (SP)
  BPL.S \60
  ROR #8,D6;format Intel
\60:MOVE D6,(A2)+;lecture en 16 bits stÇrÇo
  MOVE D6,(A2)+
  ADD.L A4,D2;pas
  ADDX.L D1,D3
 CMP.L A1,A2
 BCS.S \37
 BRA \6 
\39:ADD A5,A3
 MOVE.L A3,ADR
 BTST #1,STATUS+1
 BEQ \5;lecture sans rÇpÇtition
 BSR SWAP_ADR
 MOVE.L ADR_DEBUT,A0
 ADD TRACK,A0
 MOVE.L ADR_FIN,A6
 BRA \36
;pas de rÇÇchantillonnage
\8:LSR.L #2,D3;/4
 CMP #2,NB_VOIES
 BEQ \44;stÇrÇo
 CMP #2,OCT_ECH
 BEQ \43;16 bits mono
 ADD TRACK,A0
 MOVE NB_TRACKS,A5
 TST (SP)
 BMI \61;format Intel
 BRA \40
\41:CMP.L A6,A0
  BCC.S \42;fin de buffer
  MOVE.B (A0)+,D6
  ADD A5,A0
  ASL #8,D6
  MOVE D6,(A2)+;lecture en 16 bits stÇrÇo
  MOVE D6,(A2)+
\40:SUBQ.L #1,D3
 BPL.S \41 
 BRA \6
\42:MOVE.L A0,ADR
 BTST #1,STATUS+1
 BEQ \5;lecture sans rÇpÇtition
 BSR SWAP_ADR
 MOVE.L ADR_DEBUT,A0
 ADD TRACK,A0
 MOVE.L ADR_FIN,A6
 BRA.S \41
\76:CMP.L A6,A0
  BCC.S \77;fin de buffer
  MOVE.B (A0)+,D6
  ADD A5,A0
  ASL #8,D6
  ROR #8,D6;format Intel
  MOVE D6,(A2)+;lecture en 16 bits stÇrÇo
  MOVE D6,(A2)+
\61:SUBQ.L #1,D3
 BPL.S \76 
 BRA \6
\77:MOVE.L A0,ADR
 BTST #1,STATUS+1
 BEQ \5;lecture sans rÇpÇtition
 BSR SWAP_ADR
 MOVE.L ADR_DEBUT,A0
 ADD TRACK,A0
 MOVE.L ADR_FIN,A6
 BRA.S \76
;8 bits stÇrÇo
\45:CMP #4,OCT_ECH
 BEQ \17;16 bits stÇrÇo
 MOVE TRACK,D1
 ADD D1,A0
 ADD D1,A0
 MOVE NB_TRACKS,A5
 ADD A5,A5
 CMP.L D6,D7
 BCS \46;diminution de la longueur
;cas ou frÇquence cible > frÇquence source
;   => augmentation de la longueur
 MOVEQ #0,D0
 MOVE.L D6,D1;frÇquence source
 LSR.L #1,D1
 ROXR.L #1,D0
 MOVE.L D7,D2;rapport = frÇquence source * 2^31 / frÇquence cible
 JSR DIV64_32S
 ADD.L D0,D0;*2 => 2^31 sinon pb signe
 MOVEQ #0,D1
 MOVE NB_TRACKS,D1
 INC D1
 JSR MUL32_32
 MOVE.L D0,D1
 MOVEQ #0,D2
 MOVEQ #0,D3
\49:MOVE.L A0,A3;adresse dÇbut source
  ADD.L D3,A3
  ADD.L D3,A3
  MOVE.B (A3)+,D6;Çchantillon 8 bits voie gauche x(n)
  MOVE.B (A3)+,D4;Çchantillon 8 bits voie droite x(n)
  ASL #8,D6
  ASL #8,D4
  MOVE.L D2,D0
  SWAP D0
  LSR #1,D0;1-32767
  BEQ.S \50
  ADD A5,A3
  MOVE.B (A3)+,D7;Çchantillon 8 bits voie gauche x(n+1)
  MOVE.B (A3)+,D5;Çchantillon 8 bits voie droite x(n+1)
  ASL #8,D7
  ASL #8,D5
  MULS D0,D7
  MULS D0,D5
  NEG D0
  BCLR #15,D0;1-32767
  MULS D0,D6
  MULS D0,D4
  ADD.L D7,D6
  ADD.L D5,D4
  ADD.L D6,D6
  ADD.L D4,D4
  SWAP D6
  SWAP D4
\50:CMP.L A6,A3
  BCC.S \48;fin buffer
\47:TST (SP)
  BPL.S \62
  ROR #8,D6;format Intel
  ROR #8,D4
\62:MOVE D6,(A2)+;lecture en 16 bits stÇrÇo
  MOVE D4,(A2)+
  MOVEQ #0,D0
  ADD.L D1,D2;pas
  ADDX.L D0,D3
 CMP.L A1,A2
 BCS.S \49
 BRA \6
\48:ADD A5,A3
 MOVE.L A3,ADR
 BTST #1,STATUS+1
 BEQ \5;lecture sans rÇpÇtition
 BSR SWAP_ADR
 MOVE.L ADR_DEBUT,A0
 MOVE TRACK,D0
 ASL #1,D0
 ADD D0,A0
 MOVE.L ADR_FIN,A6
 BRA.S \47
;cas ou frÇquence cible < frÇquence source
;   => diminution de la longueur
\46:MOVEQ #0,D0
 MOVE.L D6,D1;frÇquence source
 MOVE.L D7,D2
 ASL.L #8,D2;rapport = frÇquence source * 2^31 / frÇquence cible * 2^8
 JSR DIV64_32S
 MOVE.L D0,D1
 ASL.L #8,D0
 ROL.L #8,D1
 AND.L #$FF,D1;poids fort
 JSR MUL64_TRACKS
 MOVE.L D0,A4;poids faible
 MOVEQ #0,D2
 MOVEQ #0,D3
\53:MOVE.L A0,A3;adresse dÇbut source
  ADD.L D3,A3
  ADD.L D3,A3
  MOVE.B (A3)+,D6;Çchantillon 8 bits voie gauche x(n)
  MOVE.B (A3)+,D4;Çchantillon 8 bits voie droite x(n)
  ASL #8,D6
  ASL #8,D4
  MOVE.L D2,D0
  SWAP D0
  LSR #1,D0;1-32767
  BEQ.S \54
  ADD A5,A3
  MOVE.B (A3)+,D7;Çchantillon 8 bits voie gauche x(n+1)
  MOVE.B (A3)+,D5;Çchantillon 8 bits voie droite x(n+1)
  ASL #8,D7
  ASL #8,D5
  MULS D0,D7
  MULS D0,D5
  NEG D0
  BCLR #15,D0;1-32767
  MULS D0,D6
  MULS D0,D4
  ADD.L D7,D6
  ADD.L D5,D4
  ADD.L D6,D6
  ADD.L D4,D4
  SWAP D6
  SWAP D4
\54:CMP.L A6,A3
  BCC.S \52;fin buffer
\51:TST (SP)
  BPL.S \63
  ROR #8,D6;format Intel
  ROR #8,D4
\63:MOVE D6,(A2)+;lecture en 16 bits stÇrÇo
  MOVE D4,(A2)+
  ADD.L A4,D2;pas
  ADDX.L D1,D3
 CMP.L A1,A2
 BCS.S \53
 BRA \6
\52:ADD A5,A3
 MOVE.L A3,ADR
 BTST #1,STATUS+1
 BEQ \5;lecture sans rÇpÇtition
 BSR SWAP_ADR
 MOVE.L ADR_DEBUT,A0
 MOVE TRACK,D0
 ASL #1,D0
 ADD D0,A0
 MOVE.L ADR_FIN,A6
 BRA \51
;pas de rÇÇchantillonnage
\44:CMP #4,OCT_ECH
 BEQ \27;16 bits stÇrÇo
 MOVE TRACK,D1
 ADD D1,A0
 ADD D1,A0
 MOVE NB_TRACKS,A5
 ADD A5,A5
 TST (SP)
 BMI \64;format Intel
 BRA \56
\55:CMP.L A6,A0
  BCC.S \57;fin de buffer
  MOVE.B (A0)+,D6
  MOVE.B (A0)+,D4
  ADD A5,A0
  ASL #8,D6
  ASL #8,D4
  MOVE D6,(A2)+;lecture en 16 bits stÇrÇo
  MOVE D4,(A2)+
\56:SUBQ.L #1,D3
 BPL.S \55 
 BRA \6
\57:MOVE.L A0,ADR
 BTST #1,STATUS+1
 BEQ \5;lecture sans rÇpÇtition
 BSR SWAP_ADR
 MOVE.L ADR_DEBUT,A0
 MOVE TRACK,D0
 ASL #1,D0
 ADD D0,A0
 MOVE.L ADR_FIN,A6
 BRA.S \55
\74:CMP.L A6,A0
  BCC.S \75;fin de buffer
  MOVE.B (A0)+,D6
  MOVE.B (A0)+,D4
  ADD A5,A0
  ASL #8,D6
  ASL #8,D4
  ROR #8,D6;format Intel
  ROR #8,D4
  MOVE D6,(A2)+;lecture en 16 bits stÇrÇo
  MOVE D4,(A2)+
\64:SUBQ.L #1,D3
 BPL.S \74 
 BRA \6
\75:MOVE.L A0,ADR
 BTST #1,STATUS+1
 BEQ \5;lecture sans rÇpÇtition
 BSR SWAP_ADR
 MOVE.L ADR_DEBUT,A0
 MOVE TRACK,D0
 ASL #1,D0
 ADD D0,A0
 MOVE.L ADR_FIN,A6
 BRA.S \74 
;16 bits mono
\58:MOVE TRACK,D1
 ADD D1,A0
 ADD D1,A0
 MOVE NB_TRACKS,A5
 ADD A5,A5
 CMP.L D6,D7
 BCS \22;diminution de la longueur
;cas ou frÇquence cible > frÇquence source
;   => augmentation de la longueur
 MOVEQ #0,D0
 MOVE.L D6,D1;frÇquence source
 LSR.L #1,D1
 ROXR.L #1,D0
 MOVE.L D7,D2;rapport = frÇquence source * 2^31 / frÇquence cible
 JSR DIV64_32S
 ADD.L D0,D0;*2 => 2^31 sinon pb signe
 MOVEQ #0,D1
 MOVE NB_TRACKS,D1
 INC D1
 JSR MUL32_32
 MOVE.L D0,D1
 MOVEQ #0,D2
 MOVEQ #0,D3
\19:MOVE.L A0,A3;adresse dÇbut source
  ADD.L D3,A3
  ADD.L D3,A3
  MOVE (A3)+,D6;Çchantillon 16 bits x(n)
  MOVE.L D2,D0
  SWAP D0
  LSR #1,D0;1-32767
  BEQ.S \20
  ADD A5,A3
  MOVE (A3)+,D7;Çchantillon 16 bits mono x(n+1)
  MULS D0,D7
  NEG D0
  BCLR #15,D0;1-32767
  MULS D0,D6
  ADD.L D7,D6
  ADD.L D6,D6
  SWAP D6
\20:CMP.L A6,A3
  BCC.S \21;fin buffer
\18:TST (SP)
  BPL.S \65
  ROR #8,D6;format Intel
\65:MOVE D6,(A2)+;lecture en 16 bits stÇrÇo
  MOVE D6,(A2)+
  MOVEQ #0,D0
  ADD.L D1,D2;pas
  ADDX.L D0,D3
 CMP.L A1,A2
 BCS.S \19
 BRA \6
\21:ADD A5,A3
 MOVE.L A3,ADR
 BTST #1,STATUS+1
 BEQ \5;lecture sans rÇpÇtition
 BSR SWAP_ADR
 MOVE.L ADR_DEBUT,A0
 MOVE TRACK,D0
 ASL #1,D0
 ADD D0,A0
 MOVE.L ADR_FIN,A6
 BRA.S \18
;cas ou frÇquence cible < frÇquence source
;   => diminution de la longueur
\22:MOVEQ #0,D0
 MOVE.L D6,D1;frÇquence source
 MOVE.L D7,D2
 ASL.L #8,D2;rapport = frÇquence source * 2^31 / frÇquence cible * 2^8
 JSR DIV64_32S
 MOVE.L D0,D1
 ASL.L #8,D0
 ROL.L #8,D1
 AND.L #$FF,D1;poids fort
 JSR MUL64_TRACKS
 MOVE.L D0,A4;poids faible
 MOVEQ #0,D2
 MOVEQ #0,D3
\23:MOVE.L A0,A3;adresse dÇbut source
  ADD.L D3,A3
  ADD.L D3,A3
  MOVE (A3)+,D6;Çchantillon 16 bits mono x(n)
  MOVE.L D2,D0
  SWAP D0
  LSR #1,D0;1-32767
  BEQ.S \24
  ADD A5,A3
  MOVE (A3)+,D7;Çchantillon 16 bits mono x(n+1)
  MULS D0,D7
  NEG D0
  BCLR #15,D0;1-32767
  MULS D0,D6
  ADD.L D7,D6
  ADD.L D6,D6
  SWAP D6
\24:CMP.L A6,A3
  BCC.S \25;fin buffer
\26:TST (SP)
  BPL.S \66
  ROR #8,D6;format Intel
\66:MOVE D6,(A2)+;lecture en 16 bits stÇreo
  MOVE D6,(A2)+
  ADD.L A4,D2;pas
  ADDX.L D1,D3
 CMP.L A1,A2
 BCS.S \23
 BRA \6 
\25:ADD A5,A3
 MOVE.L A3,ADR
 BTST #1,STATUS+1
 BEQ \5;lecture sans rÇpÇtition
 BSR SWAP_ADR
 MOVE.L ADR_DEBUT,A0
 MOVE TRACK,D0
 ASL #1,D0
 ADD D0,A0
 MOVE.L ADR_FIN,A6
 BRA \26
;pas de rÇÇchantillonnage
\43:MOVE TRACK,D1
 ADD D1,A0
 ADD D1,A0
 MOVE NB_TRACKS,A5
 ADD A5,A5
 BRA.S \28
\29:CMP.L A6,A0
  BCC.S \30;fin de buffer
  MOVE (A0)+,D6
  ADD A5,A0
  TST (SP)
  BPL.S \67
  ROR #8,D6;format Intel
\67:MOVE D6,(A2)+;lecture en 16 bits stÇreo
  MOVE D6,(A2)+
\28:SUBQ.L #1,D3
 BPL.S \29 
 BRA \6
\30:MOVE.L A0,ADR
 BTST #1,STATUS+1
 BEQ \5;lecture sans rÇpÇtition
 BSR SWAP_ADR
 MOVE.L ADR_DEBUT,A0
 MOVE TRACK,D0
 ASL #1,D0
 ADD D0,A0
 MOVE.L ADR_FIN,A6
 BRA.S \29
;16 bits stÇrÇo
\17:MOVE TRACK,D1
 ASL #2,D1
 ADD D1,A0
 MOVE NB_TRACKS,D1
 ASL #2,D1 
 MOVE D1,A5
 CMP.L D6,D7
 BCS \11;diminution de la longueur
;cas ou frÇquence cible > frÇquence source
;   => augmentation de la longueur
 MOVEQ #0,D0
 MOVE.L D6,D1;frÇquence source
 LSR.L #1,D1
 ROXR.L #1,D0
 MOVE.L D7,D2;rapport = frÇquence source * 2^31 / frÇquence cible
 JSR DIV64_32S
 ADD.L D0,D0;*2 => 2^31 sinon pb signe
 MOVEQ #0,D1
 MOVE NB_TRACKS,D1
 INC D1
 JSR MUL32_32
 MOVE.L D0,D1
 MOVEQ #0,D2
 MOVEQ #0,D3
\9:MOVE.L A0,A3;adresse dÇbut source
  ADD.L D3,A3
  ADD.L D3,A3
  ADD.L D3,A3
  ADD.L D3,A3
  MOVE (A3)+,D6;Çchantillon 16 bits voie gauche x(n)
  MOVE (A3)+,D4;Çchantillon 16 bits voie droite x(n)
  MOVE.L D2,D0
  SWAP D0
  LSR #1,D0;1-32767
  BEQ.S \10
  ADD A5,A3
  MOVE (A3)+,D7;Çchantillon 16 bits voie gauche x(n+1)
  MOVE (A3)+,D5;Çchantillon 16 bits voie droite x(n+1)
  MULS D0,D7
  MULS D0,D5
  NEG D0
  BCLR #15,D0;1-32767
  MULS D0,D6
  MULS D0,D4
  ADD.L D7,D6
  ADD.L D5,D4
  ADD.L D6,D6
  ADD.L D4,D4
  SWAP D6
  SWAP D4
\10:CMP.L A6,A3
  BCC.S \12;fin buffer
\16:TST (SP)
  BPL.S \68
  ROR #8,D6;format Intel
  ROR #8,D4
\68:MOVE D6,(A2)+;lecture en 16 bits stÇrÇo
  MOVE D4,(A2)+
  MOVEQ #0,D0
  ADD.L D1,D2;pas
  ADDX.L D0,D3
 CMP.L A1,A2
 BCS.S \9
 BRA \6
\12:ADD A5,A3
 MOVE.L A3,ADR
 BTST #1,STATUS+1
 BEQ \5;lecture sans rÇpÇtition
 BSR SWAP_ADR
 MOVE.L ADR_DEBUT,A0
 MOVE TRACK,D0
 ASL #2,D0
 ADD D0,A0
 MOVE.L ADR_FIN,A6
 BRA.S \16
;cas ou frÇquence cible < frÇquence source
;   => diminution de la longueur
\11:MOVEQ #0,D0
 MOVE.L D6,D1;frÇquence source
 MOVE.L D7,D2
 ASL.L #8,D2;rapport = frÇquence source * 2^31 / frÇquence cible * 2^8
 JSR DIV64_32S
 MOVE.L D0,D1
 ASL.L #8,D0
 ROL.L #8,D1
 AND.L #$FF,D1;poids fort
 JSR MUL64_TRACKS
 MOVE.L D0,A4;poids faible
 MOVEQ #0,D2
 MOVEQ #0,D3
\13:MOVE.L A0,A3;adresse dÇbut source
  ADD.L D3,A3
  ADD.L D3,A3
  ADD.L D3,A3
  ADD.L D3,A3
  MOVE (A3)+,D6;Çchantillon 16 bits voie gauche x(n)
  MOVE (A3)+,D4;Çchantillon 16 bits voie droite x(n)
  MOVE.L D2,D0
  SWAP D0
  LSR #1,D0;1-32767
  BEQ.S \14
  ADD A5,A3
  MOVE (A3)+,D7;Çchantillon 16 bits voie gauche x(n+1)
  MOVE (A3)+,D5;Çchantillon 16 bits voie droite x(n+1)
  MULS D0,D7
  MULS D0,D5
  NEG D0
  BCLR #15,D0;1-32767
  MULS D0,D6
  MULS D0,D4
  ADD.L D7,D6
  ADD.L D5,D4
  ADD.L D6,D6
  ADD.L D4,D4
  SWAP D6
  SWAP D4
\14:CMP.L A6,A3
  BCC.S \15;fin buffer
\3:TST (SP)
  BPL.S \69
  ROR #8,D6;format Intel
  ROR #8,D4
\69:MOVE D6,(A2)+;lecture en 16 bits stÇrÇo
  MOVE D4,(A2)+
  ADD.L A4,D2;pas
  ADDX.L D1,D3
 CMP.L A1,A2
 BCS.S \13
 BRA \6
\15:ADD A5,A3
 MOVE.L A3,ADR
 BTST #1,STATUS+1
 BEQ \5;lecture sans rÇpÇtition
 BSR SWAP_ADR
 MOVE.L ADR_DEBUT,A0
 MOVE TRACK,D0
 ASL #2,D0
 ADD D0,A0
 MOVE.L ADR_FIN,A6
 BRA \3
;pas de rÇÇchantillonnage
\27:MOVE TRACK,D1
 ASL #2,D1
 ADD D1,A0
 MOVE NB_TRACKS,D1
 ASL #2,D1 
 MOVE D1,A5
 TST (SP)
 BMI.S \72;format Intel
 BRA.S \4
\2:CMP.L A6,A0
  BCC.S \7;fin de buffer
  MOVE.L (A0)+,(A2)+;lecture en 16 bits stÇrÇ
  ADD A5,A0
\4:SUBQ.L #1,D3
 BPL.S \2 
 BRA \6
\7:MOVE.L A0,ADR
 BTST #1,STATUS+1
 BEQ.S \5;lecture sans rÇpÇtition
 BSR SWAP_ADR
 MOVE.L ADR_DEBUT,A0
 MOVE TRACK,D0
 ASL #2,D0
 ADD D0,A0
 MOVE.L ADR_FIN,A6
 BRA.S \2
\70:CMP.L A6,A0
  BCC.S \73;fin de buffer
  MOVEM (A0)+,D4/D6
  ADD A5,A0
  ROR #8,D4;format Intel
  ROR #8,D6
  MOVE D4,(A2)+;lecture en 16 bits stÇrÇo
  MOVE D6,(A2)+
\72:SUBQ.L #1,D3
 BPL.S \70 
 BRA.S \6
\73:MOVE.L A0,ADR
 BTST #1,STATUS+1
 BEQ.S \5;lecture sans rÇpÇtition
 BSR SWAP_ADR
 MOVE.L ADR_DEBUT,A0
 MOVE TRACK,D0
 ASL #2,D0
 ADD D0,A0
 MOVE.L ADR_FIN,A6
 BRA.S \70
\5:AND #$FFFC,STATUS;arràt lecture DMA
 BSR SDL_UNLOCKAUDIO
 MOVEQ #0,D0
 BSR ENDSOUND
 BSR SDL_CLOSEAUDIO
 CLR D_OPEN_SDL
 MOVE.L ADR_FIN,ADR
 ADDQ.L #1,COMPT_BLOC 
 BRA.S \1
\6:MOVE.L A0,ADR
 ADDQ.L #1,COMPT_BLOC 
 BSR SDL_UNLOCKAUDIO
\1:ADDQ #2,SP
 MOVEM.L (SP)+,D0-A6
 RTE

CONV_ECH:;D0: source, D1: fin source, D2: index bloc cible

 MOVEM.L D1-D6/A0-A2,-(SP)
 MOVE.L D0,A1;source dÇbut
 MOVE.L D1,D4;fin source
 CMP #NB_BUFFERS,D2
 BHI.S \5
 SUB.L A1,D4;taille en octets
 BGT.S \6
\5:MOVEQ #0,D0;plus d'Çchantillons
 BRA \1
\6:MOVEQ #0,D0
 MOVE D2,D0;index bloc cible
 MOVE.L #TAILLE_BUFFER+TAILLE_SOUND_HEADER,D1
 BSR MUL32_32
 MOVE.L D0,D6;offset bloc cible
 LEA TAB_BUFFERS,A2
 MOVEQ #0,D5
 MOVE OCT_ECH,D5
 MOVE.L D4,D0
 MOVE NB_TRACKS,D1
 INC D1
 MULU D5,D1
 BSR DIV32_32
 MOVE.L D0,D4;nbre Çchantillons
 CMP.L #TAILLE_BUFFER/4,D4
 BLE.S \7
;taille des Çchantillons supÇrieure Ö la taille du bloc
 MOVE.L #TAILLE_BUFFER/4,D4
 MOVE.L D4,D0
 MOVE NB_TRACKS,D1
 INC D1
 MULU D5,D1
 BSR MUL32_32;nbre d'octets max par buffer
 ADD.L A1,D0;+ dÇbut buffer DMA
 MOVE.L D0,ADR_SUITE
 BRA.S \8
\7:CLR.L ADR_SUITE
\8:MOVE.L D4,D0;nbre d'Çchantillons 16 bits stÇrÇo
 MOVE.L D0,22(A2,D6.L);numSamples
 MOVE.L (A2,D6.L),A0;cible
 MOVE OCT_ECH,D2
 SUBQ #2,D2
 BEQ.S \3;8 bits stÇrÇo ou 16 bits mono
 SUBQ #2,D2
 BEQ.S \4;16 bits stÇrÇo
;8 bits mono
 BSR CONV_MONO_8
 BRA.S \2
;8 bits stÇrÇo
\3:CMP #1,NB_VOIES
 BEQ.S \9
 BSR CONV_STEREO_8
 BRA.S \2
;16 bits mono
\9:BSR CONV_MONO_16
 BRA.S \2
;16 bits stÇrÇo
\4:BSR CONV_STEREO_16
\2:SUB.L A1,D0;adresse fin - adresse dÇbut = offset DMA
\1:MOVEM.L (SP)+,D1-D6/A0-A2
 RTS

CONV_MONO_8:

 MOVEM.L D1-D2/A0-A1,-(SP)
 ADD TRACK,A1
 MOVE NB_TRACKS,D1
 BRA.S \2
\1:MOVE.B (A1)+,D2
  ASL #8,D2
  MOVE D2,(A0)+
  MOVE D2,(A0)+
  ADD D1,A1
\2:SUBQ.L #1,D0
 BPL.S \1
 MOVE.L A1,D0
 MOVEM.L (SP)+,D1-D2/A0-A1
 RTS

CONV_STEREO_8:

 MOVEM.L D1-D3/A0-A1,-(SP)
 MOVE TRACK,D1
 ADD D1,D1
 ADD D1,A1
 MOVE NB_TRACKS,D1 
 ADD D1,D1 
 BRA.S \2
\1:MOVE.B (A1)+,D2
  ASL #8,D2
  MOVE.B (A1)+,D3
  ASL #8,D3
  MOVE D2,(A0)+
  MOVE D3,(A0)+
  ADD D1,A1
\2:SUBQ.L #1,D0
 BPL.S \1
 MOVE.L A1,D0
 MOVEM.L (SP)+,D1-D3/A0-A1
 RTS

CONV_MONO_16:

 MOVEM.L D1-D2/A0-A1,-(SP)
 MOVE TRACK,D1
 ADD D1,D1
 ADD D1,A1
 MOVE NB_TRACKS,D1 
 ADD D1,D1 
 BRA.S \2
\1:MOVE.B (A1)+,D2
  MOVE D2,(A0)+
  MOVE D2,(A0)+
  ADD D1,A1
\2:SUBQ.L #1,D0
 BPL.S \1
 MOVE.L A1,D0
 MOVEM.L (SP)+,D1-D2/A0-A1
 RTS

CONV_STEREO_16:

 MOVEM.L D1/A0-A1,-(SP)
 MOVE TRACK,D1
 ASL #2,D1
 ADD D1,A1
 MOVE NB_TRACKS,D1
 ASL #2,D1 
 BRA.S \2
\1:MOVE.L (A1)+,(A0)+
  ADD D1,A1
\2:SUBQ.L #1,D0
 BPL.S \1
 MOVE.L A1,D0
 MOVEM.L (SP)+,D1/A0-A1
 RTS
 
RECH_FREQ_REC:;frÇquence demandÇe dans D0
;au retour frÇquence approchÇe dans D0 au format MAC (frequence * 65536)

 MOVEM.L D1-D3/A0-A1,-(SP)
 LEA TAB_FREQ_REC,A0
 MOVE NB_FREQ_REC,D1
 BNE.S \4;liste frÇquences enregistrement supportÇes
;mini et maxi frÇquences enregistrement supportÇs
 CMP (A0),D0
 BCC.S \2
 MOVE.L (A0),D0;mini
 BRA.S \1
\2:CMP 4(A0),D0
 BCS.S \3
 MOVE.L 4(A0),D0;maxi
 BRA.S \1
\3:SWAP D0
 CLR D0
 BRA.S \1    
\4:DEC D1
 MOVEQ #-1,D3;mini
 MOVE.L A0,A1
\5:MOVEQ #0,D2
  MOVE (A0),D2
  SUB.L D0,D2;recherche frÇquence approchÇe parmi ceux disponibles
  BPL.S \7
  NEG.L D2
\7:CMP.L D3,D2
  BCC.S \6
  MOVE.L D2,D3
  MOVE.L A0,A1
\6:ADDQ #4,A0
 DBRA D1,\5
 MOVE.L (A1),D0
\1:MOVEM.L (SP)+,D1-D3/A0-A1
 RTS

SWAP_ADR_REC:

 MOVE.L D0,-(SP)
 MOVEQ #0,D0
 MOVE STATUS,D0
 LSR #2,D0
 BSR ENDSOUND_REC
 TST D_SETBUFF_REC
 BEQ.S \1;pas de nouvelle adresse => bouclage DMA
 MOVE.L NEW_ADR_DEBUT_REC,ADR_DEBUT_REC;prise en compte des nouvelles adresses DMA de SETBUFFER
 MOVE.L NEW_ADR_FIN_REC,ADR_FIN_REC
 CLR D_SETBUFF_REC
 MOVE.L #-1,COMPT_BLOC_REC;compteur buffer MAC
\1:MOVE.L (SP)+,D0
 RTS

INTER_REC:;sous MACOS, A1: buffer, D1.L: taille buffer

 MOVEM.L D0-A6,-(SP)
 BTST #2,STATUS+1
 BEQ \1;arràt enregistrement
 MOVE.L D1,D3;taille
 MOVE.L A1,A0;adresse Çchantillons source
 ADD.L D3,A1;adresse de fin source
 MOVE.L ADR_REC,A2;adresse cible
 MOVE TRACK,D0
 ASL #2,D0
 ADD D0,A2
 MOVE NB_TRACKS_REC,D0
 ASL #2,D0 
 MOVE D0,A5
 MOVE.L ADR_FIN_REC,A6
 MOVEQ #0,D6
 MOVE FREQ_REC,D6;frÇquence source format MAC poids fort
 MOVE.L FREQUENCE,D7;frÇquence cible
 CMP.L D6,D7
 BEQ \8;frÇquences identiques => pas de rÇÇchantillonnage
 BCS \11;diminution de la longueur
;cas ou frÇquence cible > frÇquence source
;   => augmentation de la longueur
 MOVEQ #0,D0
 MOVE.L D6,D1;frÇquence source
 LSR.L #1,D1
 ROXR.L #1,D0
 MOVE.L D7,D2;rapport = frÇquence source * 2^31 / frÇquence cible
 JSR DIV64_32S
 MOVE.L D0,D1
 ADD.L D1,D1;*2 => 2^31 sinon pb signe
 MOVEQ #0,D2
 MOVEQ #0,D3
\9:MOVE.L A0,A3;adresse dÇbut source
  ADD.L D3,A3
  ADD.L D3,A3
  ADD.L D3,A3
  ADD.L D3,A3
  MOVE (A3)+,D6;Çchantillon 16 bits voie gauche x(n)
  MOVE (A3)+,D4;Çchantillon 16 bits voie droite x(n)
  CMP.L A1,A3
  BCC \6
  MOVE.L D2,D0
  SWAP D0
  LSR #1,D0;1-32767
  BEQ.S \10
  MOVE (A3)+,D7;Çchantillon 16 bits voie gauche x(n+1)
  MOVE (A3)+,D5;Çchantillon 16 bits voie droite x(n+1)
  MULS D0,D7
  MULS D0,D5
  NEG D0
  BCLR #15,D0;1-32767
  MULS D0,D6
  MULS D0,D4
  ADD.L D7,D6
  ADD.L D5,D4
  ADD.L D6,D6
  ADD.L D4,D4
  SWAP D6
  SWAP D4
\10:CMP.L A6,A2
  BCC.S \12;fin buffer
\16:MOVE D6,(A2)+;enregistrement en 16 bits stÇrÇo
  MOVE D4,(A2)+
  ADD A5,A2
  MOVEQ #0,D0
  ADD.L D1,D2;pas
  ADDX.L D0,D3
 CMP.L A1,A3
 BCS.S \9
 BRA \6
\12:MOVE.L A2,ADR_REC
 BTST #3,STATUS+1
 BEQ \5;enregistrement sans rÇpÇtition
 BSR SWAP_ADR_REC
 MOVE.L ADR_DEBUT_REC,A2
 MOVE TRACK,D0
 ASL #2,D0
 ADD D0,A2
 MOVE.L ADR_FIN_REC,A6
 BRA.S \16
;cas ou frÇquence cible < frÇquence source
;   => diminution de la longueur
\11:MOVEQ #0,D0
 MOVE.L D6,D1;frÇquence source
 MOVE.L D7,D2
 ASL.L #8,D2;rapport = frÇquence source * 2^31 / frÇquence cible * 2^8
 JSR DIV64_32S
 MOVE.L D0,D1
 ASL.L #8,D0
 MOVE.L D0,A4;poids faible
 ROL.L #8,D1
 AND.L #$FF,D1;poids fort
 MOVEQ #0,D2
 MOVEQ #0,D3
\13:MOVE.L A0,A3;adresse dÇbut source
  ADD.L D3,A3
  ADD.L D3,A3
  ADD.L D3,A3
  ADD.L D3,A3
  MOVE (A3)+,D6;Çchantillon 16 bits voie gauche x(n)
  MOVE (A3)+,D4;Çchantillon 16 bits voie droite x(n)
  CMP.L A1,A3
  BCC \6
  MOVE.L D2,D0
  SWAP D0
  LSR #1,D0;1-32767
  BEQ.S \14
  MOVE (A3)+,D7;Çchantillon 16 bits voie gauche x(n+1)
  MOVE (A3)+,D5;Çchantillon 16 bits voie droite x(n+1)
  MULS D0,D7
  MULS D0,D5
  NEG D0
  BCLR #15,D0;1-32767
  MULS D0,D6
  MULS D0,D4
  ADD.L D7,D6
  ADD.L D5,D4
  ADD.L D6,D6
  ADD.L D4,D4
  SWAP D6
  SWAP D4
\14:CMP.L A6,A2
  BCC.S \15;fin buffer
\3:MOVE D6,(A2)+;enregistrement en 16 bits stÇrÇo
  MOVE D4,(A2)+
  ADD A5,A2
  ADD.L A4,D2;pas
  ADDX.L D1,D3
 CMP.L A1,A3
 BCS.S \13
 BRA \6
\15:MOVE.L A2,ADR_REC
 BTST #3,STATUS+1
 BEQ \5;enregistrement sans rÇpÇtition
 BSR SWAP_ADR_REC
 MOVE.L ADR_DEBUT_REC,A2
 MOVE TRACK,D0
 ASL #2,D0
 ADD D0,A2
 MOVE.L ADR_FIN_REC,A6
 BRA \3
;pas de rÇÇchantillonnage
\8:LSR.L #2,D3;/4
 BRA \4
\5:AND #$FFF3,STATUS;arràt enregistrement DMA
 MOVE.L RETOUR,-(SP)
 MOVEQ #0,D0
 BSR ENDSOUND_REC
 BSR SPBSTOPRECORDING
 MOVE.L (SP)+,RETOUR
 MOVE.L ADR_FIN_REC,A2
 BRA.S \6
\2:CMP.L A6,A2
  BCC.S \7;fin de buffer
  MOVE.L (A0)+,(A2)+;enregistrement en 16 bits stÇrÇo
  ADD A5,A2
\4:SUBQ.L #1,D3
 BPL.S \2 
 BRA.S \6
\7:MOVE.L A2,ADR_REC
 BTST #3,STATUS+1
 BEQ.S \5;enregistrement sans rÇpÇtition
 BSR SWAP_ADR_REC
 MOVE.L ADR_DEBUT_REC,A2
 MOVE TRACK,D0
 ASL #2,D0
 ADD D0,A2
 MOVE.L ADR_FIN_REC,A6
 BRA.S \2
\6:MOVE.L A2,ADR_REC
 ADDQ.L #1,COMPT_BLOC_REC 
\1:MOVEM.L (SP)+,D0-A6
 RTS

SWAP_ADR:

 MOVE.L D0,-(SP)
 MOVEQ #3,D0
 AND STATUS,D0
 BSR ENDSOUND
 TST D_SETBUFF
 BEQ.S \1;pas de nouvelle adresse => bouclage DMA
 MOVE.L NEW_ADR_DEBUT,ADR_DEBUT;prise en compte des nouvelles adresses DMA de SETBUFFER
 MOVE.L NEW_ADR_FIN,ADR_FIN
 CLR D_SETBUFF
 MOVE.L #-1,COMPT_BLOC;compteur buffer MAC/SDL
\1:MOVE.L (SP)+,D0
 RTS

CALLBACK16:;sous MACOS

 MOVEM.L D3/A2-A4,-(SP)
 MOVE.L $18(SP),A4;structure channel
 MOVE.L $14(SP),A2 
 MOVE 2(A2),D0;index buffer
 BGE.S \2
 AND #$FFFC,STATUS;arràt lecture DMA
 BRA \1
\2:BTST #0,STATUS+1
 BEQ \6;arràt lecture
 BTST #1,STATUS+1
 BEQ.S \4;lecture sans rÇpÇtition
;rÇpÇtition lecture DMA
 MOVE.L ADR_SUITE,D0
 BNE.S \5;suite du buffer DMA
 BSR SWAP_ADR
 MOVE.L ADR_DEBUT,D0
 MOVE.L D0,ADR_SUITE
 BRA.S \5
;lecture DMA sans rÇpÇtition
\4:MOVE.L ADR_SUITE,D0
 BEQ.S \6;plus d'autre Çchantillon => arràt lecture
\5:TST.L D0
 BMI \1;invalide
 MOVE.L ADR_FIN,D1
 MOVE 2(A2),D2;index buffer
 BSR CONV_ECH
 BEQ.S \6;plus aucun Çchantillon => arràt lecture
 ADDQ.L #1,COMPT_BLOC;compteur buffer MAC
 MOVEQ #0,D0
 MOVE 2(A2),D0;index buffer
 MOVE.L #TAILLE_BUFFER+TAILLE_SOUND_HEADER,D1
 BSR MUL32_32
 MOVE.L D0,D2
 MOVE.L A4,A0;channel
 MOVEQ #BUFFERCMD,D0;joue le son
 MOVEQ #0,D1
 ADD.L #TAB_BUFFERS,D2;buffer
 BSR MACSNDCMD
 MOVE.L A4,A0;channel
 MOVEQ #CALLBACKCMD,D0
 MOVE 2(A2),D1;index
 MOVEQ #0,D2
 BSR MACSNDCMD
 BRA.S \1
;arràt lecture
\6:TST.L COMPT_BLOC
 BPL.S \3
 CLR.L COMPT_BLOC
\3:CLR.L ADR_SUITE
 MOVE.L A4,A0;channel
 MOVEQ #BUFFERCMD,D0;joue le son
 MOVEQ #0,D1
 MOVE.L #TAB_BUFFERS+((TAILLE_BUFFER+TAILLE_SOUND_HEADER)*NB_BUFFERS),D2;buffer vide arràt son
 BSR MACSNDCMD
 MOVE.L A4,A0;channel
 MOVEQ #CALLBACKCMD,D0
 MOVEQ #-1,D1
 MOVEQ #0,D2
 BSR MACSNDCMD
 MOVEQ #0,D0
 BSR ENDSOUND
\1:MOVEM.L (SP)+,D3/A2-A4
 DC.L $4E740008;RTD #8

CALCUL_FREQ:

 MOVEM.L D1/A0,-(SP)
 MOVEQ #0,D0
 MOVE PRESCALE,D1
 BEQ.S \2
 CMP #11,D1
 BHI.S \2
 DEC D1;0-10
 TST HORLOGE
 BEQ.S \3
 ADDQ #3,D1
 ADDQ #8,D1
 TST HORLOGE2
 BEQ.S \3
 ADDQ #3,D1
 ADDQ #8,D1 
\3:ADD D1,D1
 LEA TAB_NUM_FREQ_FALCON,A0
 MOVE (A0,D1),D0
 BRA.S \1
\2:MOVE PRESCALE_STE,D1
 AND #3,D1
 ADD D1,D1
 LEA TAB_NUM_FREQ_STE,A0
 MOVE (A0,D1),D0
\1:MOVEM.L (SP)+,D1/A0
 RTS
 
_SETEXEC:

 CMP #77,D0;timer A
 BNE.S \2
 IFNE 0,DEBUG
  MOVEM.L D0/A0,-(SP)
  PRINTLINE DEBUG23
  MOVEM.L (SP)+,D0/A0
 ENDIF
 MOVE.L A0,VECT_TIMER_A
 MOVEQ #0,D0
 BRA.S \1
\2:CMP #79,D0;IO7
 BNE.S \3
 IFNE 0,DEBUG
  MOVEM.L D0/A0,-(SP)
  PRINTLINE DEBUG23
  MOVEM.L (SP)+,D0/A0
 ENDIF
 MOVE.L A0,VECT_IO7
 MOVEQ #0,D0
 BRA.S \1
\3:MOVEQ #1,D0
\1:RTS
 
_MFPINT:

 CMP #13,D0;timer A
 BNE.S \2
 IFNE 0,DEBUG
  MOVEM.L D0/A0,-(SP)
  PRINTLINE DEBUG22
  MOVEM.L (SP)+,D0/A0
 ENDIF
 MOVE.L A0,VECT_TIMER_A
 MOVEQ #0,D0
 BRA.S \1
\2:CMP #15,D0;IO7
 BNE.S \3
 IFNE 0,DEBUG
  MOVEM.L D0/A0,-(SP)
  PRINTLINE DEBUG22
  MOVEM.L (SP)+,D0/A0
 ENDIF
 MOVE.L A0,VECT_IO7
 MOVEQ #0,D0
 BRA.S \1
\3:MOVEQ #1,D0
\1:RTS

_JDISINT:

 CMP #13,D0;timer A
 BNE.S \2
 IFNE 0,DEBUG
  MOVEM.L D0/A0,-(SP)
  PRINTLINE DEBUG19
  MOVEM.L (SP)+,D0/A0
 ENDIF
 CLR D_TIMER_A
 MOVEQ #0,D0
 BRA.S \1
\2:CMP #15,D0;IO7
 BNE.S \3
 IFNE 0,DEBUG
  MOVEM.L D0/A0,-(SP)
  PRINTLINE DEBUG19
  MOVEM.L (SP)+,D0/A0
 ENDIF
 CLR D_IO7
 MOVEQ #0,D0
 BRA.S \1
\3:MOVEQ #1,D0 
\1:RTS

_JENABINT:

 CMP #13,D0;timer A
 BNE.S \2
 IFNE 0,DEBUG
  MOVEM.L D0/A0,-(SP)
  PRINTLINE DEBUG20
  MOVEM.L (SP)+,D0/A0
 ENDIF
 ST.B D_TIMER_A
 MOVEQ #0,D0
 BRA.S \1
\2:CMP #15,D0;IO7
 BNE.S \3
 IFNE 0,DEBUG
  MOVEM.L D0/A0,-(SP)
  PRINTLINE DEBUG20
  MOVEM.L (SP)+,D0/A0
 ENDIF
 ST.B D_IO7
 MOVEQ #0,D0
 BRA.S \1
\3:MOVEQ #1,D0 
\1:RTS
 
_XBTIMER:

 TST D0
 BNE.S \1;<> timer A
 IFNE 0,DEBUG
  MOVEM.L D0/A0,-(SP)
  PRINTLINE DEBUG21
  MOVEM.L (SP)+,D0/A0
 ENDIF
 TST D1
 BNE.S \2;<> arràt
 CLR COUNT_TIMER_A
 CLR D_INIT_TIMER_A
 CLR.L VECT_TIMER_A
 MOVEQ #0,D0
 BRA.S \1
\2:CMP #8,D1
 BNE.S \1;<> event count mode
 AND #$FF,D2
 MOVE D2,COUNT_TIMER_A;data = nbre de rÇpÇtitions
 MOVE D2,D_INIT_TIMER_A
 MOVE.L A0,VECT_TIMER_A
 MOVEQ #0,D0
\1:RTS

_LOCKSND:

 IFNE 0,DEBUG
  MOVEM.L D0/A0,-(SP)
  PRINTLINE DEBUG6
  MOVEM.L (SP)+,D0/A0
 ENDIF
 TST D_SND_LOCK
 BEQ.S \1
 MOVEQ #-$80,D0;erreur
 BRA.S \2
\1:MOVEQ #1,D0
 MOVE D0,D_SND_LOCK
\2:RTS

_UNLOCKSND:

 IFNE 0,DEBUG
  MOVEM.L D0/A0,-(SP)
  PRINTLINE DEBUG7
  MOVEM.L (SP)+,D0/A0
 ENDIF
 TST D_SND_LOCK
 BNE.S \1
 MOVE.L #$FFFFFF7F,D0
 BRA.S \2
\1:MOVEQ #0,D0
 MOVE D0,D_SND_LOCK
\2:RTS

_SOUNDCMD:

 IFNE 0,DEBUG
  MOVEM.L D0/D1/A0,-(SP)
  LINK A6,#-6
  CLR -2(A6)
  LEA -6(A6),A0
  MOVE D1,-(SP)
  MOVEQ #5,D1
  BSR CONV_DECI
  PRINTLINE DEBUG8
  PRINTLINE -6(A6)
  CONOUT #" "
  MOVEQ #0,D0
  MOVE (SP)+,D0
  LEA -6(A6),A0
  MOVEQ #5,D1
  BSR CONV_DECI
  PRINTLINE -6(A6)
  UNLK A6
  MOVEM.L (SP)+,D0/D1/A0
 ENDIF
 MOVEM.L D3-D4,-(SP)
 CMP #LTATTEN,D0;mode
 BEQ.S \10
 CMP #RTATTEN,D0
 BEQ.S \7
 CMP #LTGAIN,D0
 BEQ \16
 CMP #RTGAIN,D0
 BEQ \17
 CMP #ADDERIN,D0
 BEQ \15
 CMP #ADCINPUT,D0
 BEQ \20
 CMP #SETPRESCALE,D0
 BEQ \4
 CMP #SETSMPFREQ,D0
 BEQ \11;SETSMPFREQ validÇ par bit 5 du cookie '_SND'
 CMP #8,D0
 BEQ \13;SETFMT8BITS validÇ par bit 5 du cookie '_SND'
 CMP #9,D0
 BEQ \14;SETFMT16BITS validÇ par bit 5 du cookie '_SND'  
;10: SETFMT24BITS, 11: SETFMT32BITS 12-25: ATTCONTROLS
;validÇs par bit 5 du cookie '_SND'
 BRA \2;non reconnu
;ltatten
\10:LEA VOLUMES,A0
 TST D1;data voie gauche
 BPL.S \9
 MOVE (A0),D0;valeur courante
 BRA \6
\9:MOVE #255,D4
 SUB D1,D4;data voie gauche
 MOVE D4,(A0)
 BRA.S \8
;rtatten
\7:LEA VOLUMES,A0
 TST D1;data voie droite
 BPL.S \5
 MOVE 2(A0),D0;valeur courante
 BRA.S \6
\5:MOVE #255,D4
 SUB D1,D4;data voie droite
 MOVE D4,2(A0)
\8:TST D_MAC
 BNE.S \34;MagiCMac
 MOVE (A0)+,D2;voie gauche
 ADD (A0),D2;voie droite
 LSR #1,D2
 TST D_SDL
 BNE.S \35
 NEG D2
 ADD #255,D2
 LSR #6,D2;4 niveaux
 MOVE D2,VOLUME;3-2-1-0
 MOVE D4,D0
 BRA.S \6
\35:MOVE D2,D0
 LSR #1,D0;0-127
 BSR SDL_AUDIOVOLUME
 BRA.S \6
\34:MOVE 2(A0),D2;voie droite
 SWAP D2
 MOVE (A0),D2;voie gauche
 IFNE 0,DEBUG
  MOVEM.L D0/A0,-(SP)
  PRINTLINE DEBUG104
 MOVEM.L (SP)+,D0/A0
 ENDIF
 LEA CHANNEL,A0
 MOVEQ #VOLUMECMD,D0
 MOVEQ #0,D1
 BSR ISNDCMD
 MOVE D4,D0
\6:NEG D0
 ADD #255,D0
 AND #$F0,D0
 BRA \1
;ltgain
\16:LEA GAINS,A0
 TST D1;data voie gauche
 BMI.S \18
 MOVE D1,D4
 MOVE D4,(A0);voie gauche
 BRA \21
;rtgain
\17:LEA GAINS,A0
 TST D1;data voie droite
 BPL \19
\18:MOVE D0,-(SP);mode
 MOVEQ #0,D0
 MOVEQ #0,D1
 TST NB_INDEV
 BEQ.S \25
 IFNE 0,DEBUG
  MOVEM.L D0/A0,-(SP)
  PRINTLINE DEBUG120
  PRINTLINE DEBUG158
  MOVEM.L (SP)+,D0/A0
 ENDIF
 MOVE.L #siStereoInputGain,INFOTYPE
 LEA SPBGETDEVICEINFO,A0
 BSR CALLMACFUNCTION2
 MOVEM.L INFODATA,D0-D1
 MOVE #255,D3
 MOVE.L #$10000,D4
 SUB.L D4,D0
 BPL.S \22
 MOVEQ #0,D0
\22:LSR.L #7,D0;/128
 CMP D3,D0
 BCS.S \23
 MOVE D3,D0  
\23:SUB.L D4,D1
 BPL.S \24
 MOVEQ #0,D1
\24:LSR.L #7,D1;/128
 CMP D3,D1
 BCS.S \25
 MOVE D3,D1  
\25:LEA GAINS,A0
 MOVEM D0-D1,(A0)
 CMP #LTGAIN,(SP)+
 BEQ \1
 EXG D0,D1
 BRA \1
\19:MOVE D1,D4
 MOVE D4,2(A0);voie droite
\21:MOVE D0,-(SP);mode
 MOVEQ #0,D0
 MOVEQ #0,D1
 TST NB_INDEV
 BEQ.S \25
 MOVEM (A0),D0-D1
 ASL.L #7,D0;*128
 ASL.L #7,D1
 ADD.L #$10000,D0
 ADD.L #$10000,D1
 MOVEM.L D0-D1,INFODATA
 IFNE 0,DEBUG
  MOVEM.L D0/A0,-(SP)
  PRINTLINE DEBUG121
  PRINTLINE DEBUG158
  MOVEM.L (SP)+,D0/A0
 ENDIF
 MOVE.L #siStereoInputGain,INFOTYPE
 LEA SPBSETDEVICEINFO,A0
 BSR CALLMACFUNCTION2
 ADDQ #2,SP
 MOVEQ #0,D0
 MOVE D4,D0
 BRA \1
;adderin
\15:TST NB_INDEV
 BEQ \30
 TST D1;data
 BPL.S \31
 IFNE 0,DEBUG
  MOVEM.L D0/A0,-(SP)
  PRINTLINE DEBUG120
  PRINTLINE DEBUG157
  MOVEM.L (SP)+,D0/A0
 ENDIF
 MOVE.L #siPlayThruOnOff,INFOTYPE
 LEA SPBGETDEVICEINFO,A0
 BSR CALLMACFUNCTION2
 BNE.S \30;erreur
 BRA.S \32 
\31:MOVEQ #0,D0
 BTST #0,D1;ADC sur DAC (B0)
 BEQ.S \33
 MOVEQ #7,D0
\33:MOVE D0,INFODATA
 IFNE 0,DEBUG
  MOVEM.L D0/A0,-(SP)
  PRINTLINE DEBUG121
  PRINTLINE DEBUG157
  MOVEM.L (SP)+,D0/A0
 ENDIF
 MOVE.L #siPlayThruOnOff,INFOTYPE
 LEA SPBSETDEVICEINFO,A0
 BSR CALLMACFUNCTION2
 BNE.S \30;erreur 
\32:MOVE INFODATA,D0
 BNE.S \30
 MOVEQ #2,D0;Matrice
 BRA \1
\30:MOVEQ #3,D0;ADC (B0) et Matrice (B1) 
 BRA \1
;adcinput
\20:TST NB_INDEV
 BEQ \27
 TST D1;data
 BPL.S \26
 IFNE 0,DEBUG
  MOVEM.L D0/A0,-(SP)
  PRINTLINE DEBUG120
  PRINTLINE DEBUG150
  MOVEM.L (SP)+,D0/A0
 ENDIF
 MOVE.L #siInputSource,INFOTYPE
 LEA SPBGETDEVICEINFO,A0
 BSR CALLMACFUNCTION2
 BNE.S \27;erreur
 BRA.S \28
\26:MOVE INFOS_COOKIE+4,INSOURCE
 MOVEQ #1,D0
 AND #3,D1;data
 BNE.S \29
 MOVE INSOURCE,D0
\29:MOVE D0,INFODATA
 IFNE 0,DEBUG
  MOVEM.L D0/A0,-(SP)
  PRINTLINE DEBUG121
  PRINTLINE DEBUG150
  MOVEM.L (SP)+,D0/A0
 ENDIF
 MOVE.L #siInputSource,INFOTYPE
 LEA SPBSETDEVICEINFO,A0
 BSR CALLMACFUNCTION2
 BNE.S \27;erreur
\28:MOVE INFODATA,D0
 CMP INSOURCE,D0
 BEQ.S \27
 MOVEQ #3,D0;ADC input sur PSG
 BRA.S \1
\27:MOVEQ #0,D0;ADC input sur microphone
 BRA.S \1
;prescale
\4:TST D1;data
 BMI.S \3
 MOVE D1,PRESCALE_STE
 BSR CALCUL_FREQ
 MOVE.L D0,FREQUENCE
\3:MOVEQ #0,D0
 MOVE PRESCALE_STE,D0
 BRA.S \1
;setsmpfreq
\11:CMP #-1,D1;data
 BEQ.S \12
 MOVEQ #0,D0
 MOVE D1,D0
 MOVE.L D0,FREQUENCE
\12:MOVE.L FREQUENCE,D0
 BRA.S \1
;setfmt8bits
\13:MOVEQ #1,D0;signed
 BRA.S \1
;setfmt16bits
\14:MOVEQ #5,D0;signed motorola big endian
 BRA.S \1 
\2:MOVEQ #0,D0
\1:MOVEM.L (SP)+,D3-D4
 RTS

_SETBUFFER:

 IFNE 0,DEBUG
  MOVEM.L D0/A0,-(SP)
  PRINTLINE DEBUG9
  MOVEM.L (SP)+,D0/A0
 ENDIF
 TST D0
 BNE \8;enregistrement
 MOVEQ #1,D0
 AND STATUS,D0
 MOVE D0,D_SETBUFF
 BNE.S \5;DMA en activitÇ
 CLR.L ADR_SUITE
 MOVE.L D1,ADR_DEBUT;dÇmarrage
 MOVE.L D1,ADR_FIN
 BRA.S \6
\5:MOVE.L D1,NEW_ADR_DEBUT;pour prise en compte au changement de buffer DMA
 MOVE.L D1,NEW_ADR_FIN
\6:MOVE.L D2,D0
 SUB.L D1,D0
 TST.L D0
 BGT.S \2
\3:MOVEQ #1,D0;erreur
 BRA \1
\2:IFNE 0,DEBUG
  MOVEM.L D0/D1/A0,-(SP)
  LINK A6,#-12
  CLR -2(A6)
  LEA -12(A6),A0
  MOVE.L D1,D0
  MOVEQ #10,D1
  BSR CONV_DECI
  PRINTLINE DEBUG112
  PRINTLINE -12(A6)
  CLR -2(A6)
  LEA -12(A6),A0
  MOVEQ #10,D1
  MOVE.L D2,D0
  BSR CONV_DECI
  PRINTLINE DEBUG113
  PRINTLINE -12(A6)
  UNLK A6
  MOVEM.L (SP)+,D0/D1/A0
 ENDIF
 MOVEQ #1,D0
 AND STATUS,D0
 BNE.S \4;lecture DMA en activitÇ
 MOVE.L D2,ADR_FIN
 BRA \7
\4:MOVE.L D2,NEW_ADR_FIN
 BRA \7
\8:TST NB_INDEV
 BEQ \3;erreur
 CMP #1,D0;enregistrement
 BNE \3;erreur
 MOVEQ #4,D0
 AND STATUS,D0
 MOVE D0,D_SETBUFF_REC
 BNE.S \9;DMA en activitÇ
 MOVE.L D1,ADR_DEBUT_REC;dÇmarrage
 MOVE.L D1,ADR_FIN_REC
 BRA.S \10
\9:MOVE.L D1,NEW_ADR_DEBUT_REC;pour prise en compte au changement de buffer DMA
 MOVE.L D1,NEW_ADR_FIN_REC
\10:MOVE.L D2,D0
 SUB.L D1,D0
 TST.L D0
 BLE \3;erreur
 IFNE 0,DEBUG
  MOVEM.L D0/D1/A0,-(SP)
  LINK A6,#-12
  CLR -2(A6)
  LEA -12(A6),A0
  MOVE.L D1,D0
  MOVEQ #10,D1
  BSR CONV_DECI
  PRINTLINE DEBUG112
  PRINTLINE -12(A6)
  CLR -2(A6)
  LEA -12(A6),A0
  MOVEQ #10,D1
  MOVE.L D2,D0
  BSR CONV_DECI
  PRINTLINE DEBUG113
  PRINTLINE -12(A6)
  UNLK A6
  MOVEM.L (SP)+,D0/D1/A0
 ENDIF 
 MOVEQ #4,D0
 AND STATUS,D0
 BNE.S \12;enregistrement DMA en activitÇ
 MOVE.L D2,ADR_FIN_REC
 BRA.S \7
\12:MOVE.L D2,NEW_ADR_FIN_REC
\7:MOVEQ #0,D0;OK
\1:RTS

_SETMODE:

 IFNE 0,DEBUG
  MOVEM.L D0/A0,-(SP)
  PRINTLINE DEBUG11
  MOVEM.L (SP)+,D0/A0
 ENDIF
 TST D0
 BEQ.S \6;8 bits stÇrÇo
 CMP #1,D0
 BEQ.S \5;16 bits stÇrÇo
 CMP #2,D0
 BEQ.S \4;8 bits mono
 CMP #3,D0
 BEQ.S \3;16 bits mono, n'existe pas avec le XBIOS du FALCON
 MOVEQ #1,D0;erreur
 BRA.S \1
\6:MOVEQ #2,D0
 MOVE D0,NB_VOIES
 MOVE D0,OCT_ECH;8 bits stÇrÇo => 2 octets/Çchantillon
 BRA.S \2
\5:MOVE #2,NB_VOIES
 MOVE #4,OCT_ECH;16 bits stÇrÇo => 4 octets/Çchantillons
 BRA.S \2
\4:MOVEQ #1,D0
 MOVE D0,NB_VOIES
 MOVE D0,OCT_ECH;8 bits mono => 1 octet/Çchantillon
 BRA.S \2
\3:MOVE #1,NB_VOIES
 MOVE #2,OCT_ECH;16 bits mono => 2 octets/Çchantillon 
\2:MOVEQ #0,D0;OK
\1:RTS

_SETTRACKS:

 IFNE 0,DEBUG
  MOVEM.L D0/A0,-(SP)
  PRINTLINE DEBUG17
  MOVEM.L (SP)+,D0/A0
 ENDIF
 AND #3,D0
 MOVE D0,NB_TRACKS
 AND #3,D1
 MOVE D1,NB_TRACKS_REC
 MOVEQ #0,D0;OK
 RTS

_SETMONTRACKS:

 IFNE 0,DEBUG
  MOVEM.L D0/A0,-(SP)
  PRINTLINE DEBUG17
  MOVEM.L (SP)+,D0/A0
 ENDIF
 AND #3,D0
 MOVE D0,TRACK
 MOVEQ #0,D0;OK
 RTS 

_SETINTERRUPT:

 IFNE 0,DEBUG
  MOVEM.L D0/A0,-(SP)
  PRINTLINE DEBUG15
  MOVEM.L (SP)+,D0/A0
 ENDIF
 CMP #2,D0;src_inter validÇ par bit 5 du cookie '_SND' et le cookie 'GSXB'
 BNE.S \4
 CLR CAUSE_INTER
 TST D1;cause
 BEQ.S \2;disable interrupt
 CMP #1,D1
 BEQ.S \5;int_addr called on eof DAC interrupts
 CMP #2,D1
 BEQ.S \7;int_addr called on eof ADC interrupts
 CMP #3,D1
 BEQ.S \3;int_addr called on eof DAC/ADC interrupts
\8:MOVEQ #1,D0;erreur
 BRA.S \1
\4:BHI.S \8;erreur
 CMP #3,D1
 BHI.S \8;erreur
 MOVE D1,CAUSE_INTER
 TST D0;src_inter
 BEQ.S \6;timer A
 BSET #7,CAUSE_INTER;mfp
 BRA.S \6
\5:MOVE.L D2,INTH_ADDR
 BRA.S \6 
\2:MOVEQ #0,D2
\3:MOVE.L D2,INTH_ADDR
\7:MOVE.L D2,INTH_ADDR_REC
\6:MOVEQ #0,D0;OK
\1:RTS

_BUFFOPER:

 MOVEM.L D3-D4/A3,-(SP)
 IFNE 0,DEBUG
  MOVEM.L D0/A0,-(SP)
  PRINTLINE DEBUG10
  MOVEM.L (SP)+,D0/A0
 ENDIF
 TST D0;mode
 BPL \6
 IFNE 0,DEBUG
  MOVEM.L D0/D1/A0,-(SP)
  PRINTLINE DEBUG111
  LINK A6,#-6
  CLR -2(A6)
  LEA -6(A6),A0
  MOVEQ #5,D1
  MOVEQ #0,D0
  MOVE STATUS,D0
  BSR CONV_DECI
  PRINTLINE -6(A6)
  UNLK A6
  MOVEM.L (SP)+,D0/D1/A0
 ENDIF
 MOVEQ #0,D0
 MOVE STATUS,D0;Çtat
 BRA \1
\6:TST NB_INDEV
 BEQ \5;test lecture
 MOVE D0,-(SP)
 BTST #2,D0;enregistrement DMA
 BEQ \11
;enregistrement DMA
 BTST #2,STATUS+1;enregistrement DMA
 BNE \10;DMA dÇja en marche 
 CLR.L COMPT_BLOC_REC;compteur buffer MAC
 IFNE 0,DEBUG
  MOVEM.L D0/A0,-(SP)
  PRINTLINE DEBUG121
  MOVEM.L (SP)+,D0/A0
 ENDIF
 MOVE.L FREQUENCE,D0
 BSR RECH_FREQ_REC
 MOVE.L D0,FREQ_REC
 MOVE.L D0,INFODATA
 MOVE.L #siSampleRate,INFOTYPE
 LEA SPBSETDEVICEINFO,A0
 BSR CALLMACFUNCTION2 
 BNE \10;erreur
 IFNE 0,DEBUG
  MOVEM.L D0/A0,-(SP)
  PRINTLINE DEBUG122
  MOVEM.L (SP)+,D0/A0
 ENDIF
 LEA INPARAMPTR,A1
 MOVE.L ADR_DEBUT_REC,ADR_REC
 MOVE.L INREFNUM,_INREFNUM(A1)
 CLR.L _COUNTREC(A1)
 CLR.L _MILLISECONDS(A1)
 CLR.L _BUFFERLENGTH(A1)
 CLR.L _BUFFERPTR(A1)
 CLR.L _COMPLEXIONROUTINE(A1)
 LEA INTER_REC,A0
 MOVE.L A0,_INTERRUPTROUTINE(A1)
 CLR.L _USERLONG(A1)
 LEA SPBRECORD,A0
 BSR CALLMACFUNCTION2
 BNE.S \10;erreur
 MOVEQ #$C,D0
 AND (SP),D0
 AND #$FFF3,STATUS
 OR D0,STATUS;enregistrement DMA
 BRA.S \10
\11:BTST #2,STATUS+1;enregistrement DMA
 BEQ.S \10;DMA dÇjÖ l'arràt 
 AND #$FFF3,STATUS;arràt enregistrement DMA
 CLR D_SETBUFF_REC
 BSR STOP_RECORD
\10:MOVE (SP)+,D0
\5:BTST #0,D0;lecture DMA
 BEQ \3;demande DMA Ö l'arràt
;lecture DMA
 BTST #0,STATUS+1
 BNE \8;DMA dÇja en marche
 MOVEQ #3,D1
 AND D0,D1
 AND #$FFFC,STATUS
 OR D1,STATUS
 CLR.L COMPT_BLOC;compteur buffer MAC/SDL
 TST D_MAC
 BNE \12;MagiCMac
 TST D_SDL
 BEQ.S \19
 TST D_OPEN_SDL
 BNE \3
 CLR D_INTEL_SDL
 MOVE.L FREQUENCE,D0
 MOVE #AUDIO_S16MSN,D1
 MOVEQ #2,D2;stÇrÇo
 MOVE #TAILLE_BUFFER_SDL/4,D3
 LEA TAB_BUFFERS,A0
 BSR SDL_OPENAUDIO
 BMI \3;erreur
 CMP #AUDIO_S16MSN,D0
 BEQ.S \18
 CMP #AUDIO_S16LSB,D0
 BNE \3;erreur 
 ST.B D_INTEL_SDL
\18:ST.B D_OPEN_SDL
 BSR SDL_GETAUDIOFREQ
 MOVE.L D0,FREQ_LECT
 MOVE.L ADR_DEBUT,ADR
 SETEXEC CALLBACK16_SDL,#29;INT5
 MOVE.L D0,CALLBACK16_SDL-4 
 MOVEQ #0,D0
 BSR SDL_PAUSEAUDIO;lance la lecture
 BRA \2
\19:MOVEQ #0,D1
 CMP #2,NB_VOIES
 BEQ.S \14;stÇrÇo
 CMP #2,OCT_ECH
 BEQ.S \16;16 bits mono
 BRA.S \15;8 bits mono
\14:BSET #0,D1;stÇrÇo
 CMP #4,OCT_ECH
 BNE.S \15
\16:BSET #1,D1;16 bits
\15:MOVE.L FREQUENCE,D2
 BTST #1,D0;rÇpÇtition
 BEQ.S \13
 BSET #6,D1;rÇpÇtition
\13:MOVE.L ADR_DEBUT,A0
 MOVE.L ADR_FIN,D0
 SUB.L A0,D0
 TIMER_OUTPUT D1,D2,(A0),D0
 BRA \2
\12:MOVE.L #-1,ADR_SUITE;invalide
 BSR STOP_SOUND
 IFNE 0,DEBUG
  MOVEM.L D0/D1/A0,-(SP)
  PRINTLINE DEBUG112
  LINK A6,#-12
  CLR -2(A6)
  LEA -12(A6),A0
  MOVEQ #10,D1
  MOVE.L ADR_DEBUT,D0
  BSR CONV_DECI
  PRINTLINE -12(A6)
  PRINTLINE DEBUG113
  CLR -2(A6)
  LEA -12(A6),A0
  MOVEQ #10,D1
  MOVE.L ADR_FIN,D0
  BSR CONV_DECI
  PRINTLINE -12(A6)
  PRINTLINE DEBUG114
  CLR -2(A6)
  LEA -12(A6),A0
  MOVEQ #10,D1
  MOVE.L FREQUENCE,D0
  BSR CONV_DECI
  PRINTLINE -12(A6)
  PRINTLINE DEBUG115
  CLR -2(A6)
  LEA -6(A6),A0
  MOVEQ #5,D1
  MOVEQ #0,D0
  MOVE OCT_ECH,D0
  BSR CONV_DECI
  PRINTLINE -6(A6)
  UNLK A6
  MOVEM.L (SP)+,D0/D1/A0
 ENDIF
 LEA TAB_BUFFERS,A3
 MOVE.L ADR_DEBUT,D4
 MOVEQ #0,D3;index bloc
\4:CMP #1,D3
  BCS.S \9;1er bloc vide
  MOVE.L D4,D0;offset bloc
  MOVE.L ADR_FIN,D1
  MOVE D3,D2;index bloc cible
  BSR CONV_ECH
  BEQ \7;plus d'Çchantillons
  ADD.L D0,D4
\9:MOVE.L FREQUENCE,D1
  SWAP D1
  MOVE.L D1,8(A3);frÇquence * 65536
  IFNE 0,DEBUG
   MOVEM.L D0/A0,-(SP)
   PRINTLINE DEBUG102
   MOVEM.L (SP)+,D0/A0  
  ENDIF
  LEA CHANNEL,A0
  MOVEQ #BUFFERCMD,D0;joue le son
  MOVEQ #0,D1
  MOVE.L A3,D2;buffer
  BSR SNDCMD
  IFNE 0,DEBUG
   MOVEM.L D0/A0,-(SP)
   PRINTLINE DEBUG103
   MOVEM.L (SP)+,D0/A0
  ENDIF
  LEA CHANNEL,A0
  MOVEQ #CALLBACKCMD,D0
  MOVE D3,D1;index bloc
  MOVEQ #0,D2
  BSR SNDCMD
  ADD.L #TAILLE_BUFFER+TAILLE_SOUND_HEADER,A3
  INC D3
 CMP #NB_BUFFERS,D3
 BCS \4
 MOVE.L FREQUENCE,D1
 SWAP D1
 MOVE.L D1,8(A3);frÇquence * 65536 buffer vide arràt son
 BRA.S \2
\7:TST D3
 BNE.S \2
 AND #$FFFC,STATUS;arràt lecture DMA si plus d'Çchantillons au 1er bloc
 BRA.S \2
;DMA Ö l'arràt
\3:AND #$FFFC,STATUS;arràt lecture DMA
 TST D_MAC
 BEQ.S \17;<> MagiCMac
 BTST #1,D0
 BNE.S \2;DMA en rÇpÇtition
\17:CLR D_SETBUFF
 BSR STOP_SOUND
\2:IFNE 0,DEBUG
  MOVEM.L D0/A0,-(SP)
  PRINTLINE DEBUG110
  MOVEM.L (SP)+,D0/A0
 ENDIF
\8:MOVEQ #0,D0;OK
\1:MOVEM.L (SP)+,D3-D4/A3
 RTS
 
_GPIO:

 IFNE 0,DEBUG
  MOVEM.L D0/A0,-(SP)
  PRINTLINE DEBUG16
  MOVEM.L (SP)+,D0/A0
 ENDIF
 CMP #1,D0;mode: lecture
 BNE.S \2
 MOVEQ #1,D0
 AND.B HORLOGE2,D0;GPIO0=0 44.1KHz
 BSET #1,D0;GPIO1=1 Quartz
 BRA.S \1
\2:CMP #2,D0;mode: Çcriture
 BNE.S \1
 BTST #0,D1;data B0: 0: horloge 22.5792 Mhz pour 44.1KHz, 1: horloge 24,576 MHz pour 48KHZ
 SNE.B HORLOGE2
\1:MOVEQ #0,D0;OK
 RTS

_DEVCONNECT:

 IFNE 0,DEBUG
  MOVEM.L D0/A0,-(SP)
  PRINTLINE DEBUG12
  MOVEM.L (SP)+,D0/A0
 ENDIF
 TST D0
 BEQ.S \4;source = restitution DMA
 TST NB_INDEV
 BEQ.S \3;erreur
 CMP #3,D0
 BEQ.S \4;source = ADC
 CMP #2,D2;srcclk
 BCS.S \3;horloge 25.175 Mhz (0) ou horloge externe (1)
 MOVE D2,HORLOGE
 MOVE 4(SP),PRESCALE
 BSR CALCUL_FREQ
 MOVE.L D0,FREQUENCE
 BRA.S \3 
\4:CMP #2,D2;srcclk
 BCS.S \2;horloge 25.175 Mhz (0) ou horloge externe (1)
;horloge 32 MHz
\3:MOVEQ #1,D0;erreur
 BRA.S \1
\2:MOVE D2,HORLOGE
 MOVE 4(SP),PRESCALE
 BSR CALCUL_FREQ
 MOVE.L D0,FREQUENCE
 IFNE 0,DEBUG
  MOVEM.L D1/A0,-(SP)
  LINK A6,#-6
  CLR -2(A6)
  LEA -6(A6),A0
  MOVEQ #5,D1
  BSR CONV_DECI
  PRINTLINE -6(A6)
  UNLK A6
  MOVEM.L (SP)+,D1/A0
 ENDIF
 MOVEQ #0,D0;OK
\1:RTS

_SNDSTATUS:

 IFNE 0,DEBUG
  MOVEM.L D0/A0,-(SP)
  PRINTLINE DEBUG13
  MOVEM.L (SP)+,D0/A0
 ENDIF
 TST D0
 BEQ \1;status
 CMP #1,D0
 BEQ.S \2;reset
 CMP #2,D0
 BEQ \3;resolutions validÇ par bit 5 du cookie '_SND'
 CMP #3,D0
 BEQ \6;MasterMix
;4: record sources, 5: duplex
 CMP #8,D0
 BEQ \4;format8bits validÇ par bit 5 du cookie '_SND'
 CMP #9,D0
 BEQ \5;format16bits validÇ par bit 5 du cookie '_SND'
;10: format24bits, 11: format32bits
 MOVEQ #0,D0 
 BRA \1
;reset
\2:BTST #2,STATUS+1;enregistrement DMA
 BEQ.S \7
 BSR STOP_RECORD
\7:CLR STATUS
 CLR D_SETBUFF
 CLR D_SETBUFF_REC
 BSR STOP_SOUND
 MOVEQ #LTATTEN,D0;mode 
 MOVEQ #0,D1;data
 BSR _SOUNDCMD
 MOVEQ #RTATTEN,D0;mode 
 MOVEQ #0,D1;data
 BSR _SOUNDCMD
 MOVEQ #LTGAIN,D0;mode 
 MOVEQ #0,D1;data
 BSR _SOUNDCMD
 MOVEQ #RTGAIN,D0;mode 
 MOVEQ #0,D1;data
 BSR _SOUNDCMD
 MOVEQ #ADDERIN,D0;mode
 MOVEQ #1,D1;data
 BSR _SOUNDCMD
 MOVEQ #ADCINPUT,D0;mode
 MOVEQ #0,D1;data
 BSR _SOUNDCMD
 CLR.L ADR_SUITE
 CLR.L ADR_FIN
 CLR.L ADR_DEBUT
 CLR.L NEW_ADR_DEBUT
 CLR.L NEW_ADR_FIN
 CLR.L ADR_FIN_REC
 CLR.L ADR_DEBUT_REC
 CLR.L NEW_ADR_DEBUT_REC
 CLR.L NEW_ADR_FIN_REC
 CLR PRESCALE
 CLR PRESCALE_STE
 CLR HORLOGE
 CLR HORLOGE2
 CLR NB_TRACKS
 CLR NB_TRACKS_REC
 CLR TRACK
 CLR.L COMPT_BLOC;compteur buffer MAC
 MOVEQ #2,D0
 MOVE D0,NB_VOIES
 MOVE D0,OCT_ECH;8 bits stÇrÇo
 CLR.L INTH_ADDR;routine appelÇe en fin lecture buffer DMA
 CLR.L INTH_ADDR_REC;routine appelÇe en fin enregistrement buffer DMA
 CLR CAUSE_INTER;setinterrupt
 MOVEQ #0,D0
 BRA.S \1
;resolutions
\3:MOVEQ #3,D0;8 & 16 bits
 BRA.S \1
;MasterMix
\6:MOVEQ #3,D0;DAC & ADC
 BRA.S \1
;format8bits
\4:MOVEQ #1,D0;signed
 BRA.S \1
;format16bits
\5:MOVEQ #5,D0;signed motorola big endian
\1:RTS

_BUFFPTR:

 MOVE.L D3,-(SP)
 IFNE 0,DEBUG
  MOVEM.L D0/A0,-(SP)
  PRINTLINE DEBUG14
  MOVEM.L (SP)+,D0/A0
 ENDIF
 MOVE.L D0,A0
 BTST #0,STATUS+1
 BEQ \2;lecture DMA Ö l'arràt
 TST D_MAC
 BNE.S \11;MagiCMac
 MOVE.L #TAILLE_BUFFER_SDL/4,D3
 TST D_SDL
 BNE.S \12
 MOVE.L ADRESSE,D1;adresse courante DMA (interruption timer A)
 BRA \4
\11:MOVE.L #TAILLE_BUFFER/4,D3
\12:MOVE.L D3,D1
 MOVE.L COMPT_BLOC,D0;compteur buffer MAC/SDL
 SUBQ.L #1,D0
 BPL.S \5
 MOVEQ #0,D0
\5:MOVE.L D0,D2
 BSR MUL32_32;offset buffer XBIOS en Çchantillons
 MOVE NB_TRACKS,D1
 INC D1
 MULU OCT_ECH,D1
 BSR MUL32_32;nbre d'octets lecture DMA XBIOS
 MOVE.L D0,D1
 ADD.L ADR_DEBUT,D1;+ adresse DMA dÇbut => adresse courante DMA
 BTST #1,STATUS+1
 BEQ.S \4;pas de rÇpÇtition
 MOVE.L ADR_FIN,D0
 CMP.L D0,D1
 BLE.S \4
 SUB.L ADR_DEBUT,D1;dÇpassement + taille bloc
 SUBQ.L #1,D3
 AND.L D3,D2;masque
 ASL.L #2,D2;COMPT_BLOC * 4
 SUB.L D2,D1;Çvite de retourner la valeur prÇcÇdente en cas de bouclage DMA sur un petit buffer
 BPL.S \6
 ADD.L D2,D1
\6:SUB.L ADR_DEBUT,D0;taille du bloc DMA
 EXG.L D0,D1
 MOVEQ #0,D2;dÇpassement/taille bloc DMA <=> ADR_FIN - ADR_DEBUT
 DC.L $4C410402;DIVU.L D1,D2:D0;reste dans D2
 MOVE.L D2,D1
 ADD.L ADR_DEBUT,D1;adresse courante DMA
\4:CMP.L ADR_FIN,D1
 BLE.S \3
 MOVE.L ADR_FIN,D1
 BRA.S \1
\3:CMP.L ADR_DEBUT,D1
 BGE.S \1
\2:MOVE.L ADR_DEBUT,D1
\1:MOVE.L D1,(A0)+;pointeur lecture
 TST NB_INDEV
 BEQ.S \8;pas d'enregistrement
 BTST #2,STATUS+1
 BEQ.S \8;enregistrement DMA Ö l'arràt
 MOVE.L ADR_REC,D1
 BTST #3,STATUS+1
 BEQ.S \10;pas de rÇpÇtition
 MOVE.L COMPT_BLOC_REC,D2;compteur buffer MAC
 AND.L #(TAILLE_BUFFER/4)-1,D2
 ASL.L #2,D2;COMPT_BLOC * 4
 SUB.L D2,D1;Çvite de retourner la valeur prÇcÇdente en cas de bouclage DMA sur un petit buffer
 BPL.S \10
 ADD.L D2,D1
\10:CMP.L ADR_FIN_REC,D1
 BLE.S \9
 MOVE.L ADR_FIN_REC,D1
 BRA.S \7
\9:CMP.L ADR_DEBUT_REC,D1
 BGE.S \7
\8:MOVE.L ADR_DEBUT_REC,D1
\7:MOVE.L D1,(A0);pointeur enregistrement
 IFNE 0,DEBUG
  LINK A6,#-10
  MOVEM.L D1/A0,-(SP)
  CLR -2(A6)
  MOVE.L -4(A0),D0
  SUB.L ADR_DEBUT,D0
  LEA -10(A6),A0
  MOVEQ #9,D1
  BSR CONV_DECI
  PRINTLINE DEBUG117
  PRINTLINE -10(A6)
  MOVE.L ADR_FIN,D0
  SUB.L ADR_DEBUT,D0
  LEA -10(A6),A0
  MOVEQ #9,D1
  BSR CONV_DECI
  PRINTLINE -10(A6)
  MOVE.L COMPT_BLOC,D0 
  LEA -6(A6),A0
  MOVEQ #5,D1
  BSR CONV_DECI
  PRINTLINE -6(A6)
  MOVEM.L (SP),D1/A0      
  MOVE.L D1,D0
  SUB.L ADR_DEBUT_REC,D0
  LEA -10(A6),A0
  MOVEQ #9,D1
  BSR CONV_DECI
  PRINTLINE -10(A6)
  MOVE.L ADR_FIN_REC,D0
  SUB.L ADR_DEBUT_REC,D0
  LEA -10(A6),A0
  MOVEQ #9,D1
  BSR CONV_DECI
  PRINTLINE -10(A6)
  MOVE.L COMPT_BLOC_REC,D0 
  LEA -6(A6),A0
  MOVEQ #5,D1
  BSR CONV_DECI
  PRINTLINE -6(A6)   
  LEA -6(A6),A0
  MOVEQ #5,D1
  MOVEQ #0,D0
  MOVE STATUS,D0
  BSR CONV_DECI
  PRINTLINE DEBUG116
  PRINTLINE -6(A6)  
  MOVEM.L (SP)+,D1/A0  
  UNLK A6
 ENDIF  
 MOVE.L (SP)+,D3
 MOVEQ #0,D0;OK
 RTS

 DC.L "XBRA"
 DC.L "MgSn"
 DC.L 0

DET_BIOS:

 MOVE.L USP,A0
 BTST #5,(SP);appel en superviseur
 BEQ.S \2
 LEA 6(SP),A0
 TST PROC_TYPE
 BEQ.S \2
 TST (A0)+;mot supplÇmentaire sur la pile si > 68000
\2:MOVE (A0)+,D0;fonction
 CMP #5,D0;setexec
 BNE.S \1
 MOVE (A0)+,D0;num
 MOVE.L (A0),A0;vector
 BSR _SETEXEC
 BNE.S \1
 TST D_MAC
 BEQ.S \1
 RTE
\1:MOVE.L DET_BIOS-4,-(SP)
 RTS

 DC.L "XBRA"
 DC.L "MgSn"
 DC.L 0

DET_XBIOS:

 MOVE.L USP,A0
 BTST #5,(SP);appel en superviseur
 BEQ.S \14
 LEA 6(SP),A0
 TST PROC_TYPE
 BEQ.S \14
 TST (A0)+;mot supplÇmentaire sur la pile si > 68000
\14:MOVE (A0)+,D0;fonction
 CMP #$D,D0;MFPINT
 BNE.S \22
 MOVE (A0)+,D0;num
 MOVE.L (A0),A0;vector
 BSR _MFPINT
 BNE \13
 BRA.S \23
\22:CMP #$1A,D0;JDISINT
 BNE.S \19
 MOVE (A0),D0;num
 BSR _JDISINT
 BNE \13
 BRA.S \23
\19:CMP #$1B,D0;JENABINT
 BNE.S \20
 MOVE (A0),D0;num
 BSR _JENABINT
 BNE \13
 BRA.S \23
\20:CMP #$1F,D0;XBTIMER
 BNE.S \21
 MOVEM (A0),D0-D2;timer, control, data
 MOVE.L 6(A0),A0;vector
 BSR _XBTIMER
 BNE \13
\23:TST D_MAC
 BNE \1;MagiCMac
 TST D_TT
 BNE \13;routine d'origine
 bra \1
 TST D_SND_LOCK
 BNE \1
 BRA \13
\21:CMP #$80,D0;LOCKSND
 BCS \13
 CMP #$8D,D0;BUFFPTR
 BHI \13
 MOVE D0,D1
 SUB #$80,D1
 ADD D1,D1
 MOVE \3(PC,D1),D1
 JMP \3(PC,D1)
\3:DC.W \4-\3;$80 LOCKSND
 DC.W \5-\3;$81 UNLOCKSND
 DC.W \6-\3;$82 SOUNDCMD
 DC.W \7-\3;$83 SETBUFFER
 DC.W \8-\3;$84 SETMODE
 DC.W \17-\3;$85 SETTRACKS
 DC.W \18-\3;$86 SETMONTRACKS
 DC.W \15-\3;$87 SETINTERRUPT
 DC.W \9-\3;$88 BUFFOPER
 DC.W \2-\3;$89 /DSPTRISTATE
 DC.W \16-\3;$8A GPIO
 DC.W \10-\3;$8B DEVCONNECT
 DC.W \11-\3;$8C SNDSTATUS
 DC.W \12-\3;$8D BUFFPTR
\4:BSR _LOCKSND
 BRA \1
\5:BSR _UNLOCKSND
 BRA.S \1
\6:MOVEM (A0),D0-D1;mode, data
 BSR _SOUNDCMD
 BRA.S \1
\7:MOVE (A0)+,D0;reg
 MOVEM.L (A0),D1-D2;begaddr, endaddr
 BSR _SETBUFFER
 BRA.S \1
\8:MOVE (A0),D0;mode
 BSR _SETMODE
 BRA.S \1
\17:MOVEM (A0),D0-D1;playtracks, rectracks
 BSR _SETTRACKS
 BRA.S \1
\18:MOVE (A0),D0;track
 BSR _SETMONTRACKS 
 BRA.S \1
\15:MOVE (A0)+,D0;src_inter
 MOVE (A0)+,D1;cause
 MOVE.L (A0)+,D2;int_adr
 BSR _SETINTERRUPT
 BRA.S \1
\9:MOVE (A0),D0;mode
 BSR _BUFFOPER
 BRA.S \1
\16:MOVEM (A0),D0-D1;mode, data
 BSR _GPIO
 BRA.S \1
\10:SUBQ #4,SP
 MOVE (A0)+,D0;src
 MOVE (A0)+,D1;dest
 MOVE (A0)+,D2;srcclk
 MOVE (A0)+,2(SP);prescale
 MOVE (A0),(SP);protocol
 MOVE (SP),-(SP)
 MOVE 4(SP),-(SP)
 BSR _DEVCONNECT
 ADDQ #8,SP
 BRA.S \1
\11:MOVE (A0),D0;reset
 BSR _SNDSTATUS
 BRA.S \1
\12:MOVE.L (A0),D0
 BSR _BUFFPTR
 BRA.S \1
\2:MOVEQ #0,D0
\1:RTE
\13:MOVE.L DET_XBIOS-4,-(SP)
 RTS
 
RAZ_BUFFERS:

 MOVEM.L D0-D1/A0,-(SP)
 LEA TAB_BUFFERS,A0
 MOVEQ #NB_BUFFERS-1,D1
\1:LEA TAILLE_SOUND_HEADER(A0),A0
  MOVE.L #(TAILLE_BUFFER/4)-1,D0
\2:CLR.L (A0)+
  DBRA D0,\2
 DBRA D1,\1
 MOVEM.L (SP)+,D0-D1/A0
 RTS

STOP_SOUND:

 TST D_MAC
 BNE.S \2;MagiCMac
 TST D_SDL
 BNE.S \1
 BSR FIN_IRQ_PSG
 BRA.S \1
\2:IFNE 0,DEBUG
  MOVEM.L D0/A0,-(SP)
  PRINTLINE DEBUG100
  MOVEM.L (SP)+,D0/A0
 ENDIF
 LEA CHANNEL,A0
 MOVEQ #FLUSHCMD,D0;nettoyage canal
 MOVEQ #0,D1
 MOVEQ #0,D2
 BSR ISNDCMD
 IFNE 0,DEBUG
  MOVEM.L D0/A0,-(SP)
  PRINTLINE DEBUG101
  MOVEM.L (SP)+,D0/A0
 ENDIF
 LEA CHANNEL,A0
 MOVEQ #QUIETCMD,D0;arràt son
 MOVEQ #0,D1
 MOVEQ #0,D2
 BSR ISNDCMD
 BSR RAZ_BUFFERS
\1:RTS
 
STOP_RECORD:

 TST NB_INDEV
 BEQ.S \1
 IFNE 0,DEBUG
  MOVEM.L D0/A0,-(SP)
  PRINTLINE DEBUG123
  MOVEM.L (SP)+,D0/A0
 ENDIF
 LEA SPBSTOPRECORDING,A0
 BSR CALLMACFUNCTION2 
\1:RTS

 DC.L "XBRA"
 DC.L "MgSn"
 DC.L 0
 
DET_FIN_SON:

 CMP #2,D0
 BNE.S \1
 MOVEM.L D0-D7/A0-A6,-(SP)
 LEA CHANNEL,A0
 MOVE.L A0,SNDCHANNELPTR
 LEA SNDDISPOSECHANNEL,A0
 BSR CALLMACFUNCTION2
 MOVEM.L (SP)+,D0-D7/A0-A6
\2:TST.L DET_FIN_SON-4
 BEQ.S \1
 MOVE.L DET_FIN_SON-4,-(SP)
\1:RTS

INIT_COOKIE:;nom dans D0.L
;retourne le pointeur sur donnÇe cookie dans D0 ou < 0 si erreur

 MOVEM.L D1-A1/A6,-(SP)
 MOVE.L D0,D7
 BSR GET_COOKIE
 BPL \1;existe dÇja
 SUB.L A6,A6
 SUPER 1
 TST.L D0
 BMI.S \8;mode superviseur
 SUPER 0
 MOVE.L D0,A6;sauve SSP
\8:MOVE.L _COOKIE,D0
 BEQ \2;le Cookie Jar n'existe pas TOS < 1.6
 MOVE.L D0,A0
 MOVEQ #1,D0;compteur nombre de cookies
\3:TST.L (A0)
  BEQ.S \4;dernier cookie
  ADDQ #8,A0
  INC D0
 BRA.S \3
\4:CMP.L 4(A0),D0
 BCC.S \5;plus de place
 CLR.L 8(A0);id dernier cookie
 MOVE.L 4(A0),12(A0);dÇplace data dernier cookie (nombre max de cookie)
 MOVE.L D7,(A0)+;installe nouveau cookie
 MOVE.L A0,D7
 CLR.L (A0);contenu initialisÇ Ö 0
 BRA \9
\5:MOVE.L 4(A0),D1;taille
 BEQ \10;erreur
 MOVE.L D1,D2
 ASL.L #4,D2;double la taille (en octets)
 MOVE.L D2,D0
 BSR MALLOC_D0
 BMI \10;erreur
 MOVE.L _COOKIE,A0
 MOVE.L D0,A1
 MOVE.L D1,D2
 ADD.L D1,D1
 SUBQ.L #2,D2;recopie
 BMI.S \7
\6:MOVE.L (A0)+,(A1)+;id
  MOVE.L (A0)+,(A1)+;contenu
 SUBQ.L #1,D2
 BPL.S \6
 BRA.S \7
\2:MOVEQ #64,D0
 BSR MALLOC_D0;rÇserve la place pour 8 cookie si le Cookie Jar n'existe pas
 BMI.S \10;erreur
 MOVE.L D0,_COOKIE
 LEA DET_RESET,A0
 MOVE.L RESVECTOR,-4(A0)
 MOVE.L RESVALID,SAUVE_RESVALID
 MOVE.L A0,RESVECTOR;dÇtourne reset pour raz vecteur Cookie Jar
 MOVE.L #$31415926,RESVALID
 MOVE.L D0,A1
 MOVEQ #8,D1
\7:MOVE.L D7,(A1)+;installe nouveau cookie
 MOVE.L A1,D7
 CLR.L (A1)+;contenu initialisÇ Ö 0
 CLR.L (A1)+;id dernier cookie
 MOVE.L D1,(A1);taille
 BRA.S \9
\10:MOVEQ #-1,D7;erreur
\9:MOVE.L A6,D0
 BEQ.S \11
 SUPER (A6)
\11:MOVE.L D7,D0
\1:MOVEM.L (SP)+,D1-A1/A6
 RTS

GET_COOKIE:;nom dans D0.L, au retour si < 0 pas de cookie
;retourne le pointeur sur donnÇe cookie dans D0

 MOVEM.L D1-A1/A6,-(SP)
 MOVE.L D0,D6
 SUB.L A6,A6
 SUPER 1
 TST.L D0
 BMI.S \5;mode superviseur
 SUPER 0
 MOVE.L D0,A6;sauve SSP
\5:MOVEQ #0,D7
 MOVE.L _COOKIE,D0
 BEQ.S \1
 MOVE.L D0,A0
\2:TST.L (A0)
  BEQ.S \1
  CMP.L (A0),D6
  BNE.S \3
  MOVE.L A0,D7
  ADDQ #4,D7
  BRA.S \1
\3:ADDQ #8,A0
 BRA.S \2
\1:MOVE.L A6,D0
 BEQ.S \4
 SUPER (A6)
\4:MOVEQ #-1,D6
 MOVE.L D7,D0
 BEQ.S \6
 MOVEQ #0,D6
\6:TST D6
 MOVEM.L (SP)+,D1-A1/A6
 RTS

 DC.L "XBRA"
 DC.L "MgSn"
 DC.L 0;adresse de retour

DET_RESET:

 CLR.L _COOKIE;vecteur Cookie Jar
 MOVE.L SAUVE_RESVALID,RESVALID
 MOVE.L DET_RESET-4,RESVECTOR
 JMP (A6)

MALLOC_D0:

 MOVE.L A0,-(SP)
 MOVE.L D0,-(SP)
 MALLOC #-1
 MOVE.L (SP)+,A0
 CMP.L D0,A0
 BCS.S \1
\3:MOVEQ #-39,D0
 BRA.S \2
\1:MALLOC A0
 TST.L D0
 BLE.S \3
\2:MOVE.L (SP)+,A0
 TST.L D0
 RTS

CONV_DECI:;A0:pointeur cible chaåne ASCII, D0:valeur sur 32 bits, D1:nombre de chiffres

 BSR CONV_DECI_SIMPLE
 DEC D1
 BEQ.S \1
 SWAP D0
 TST D0
 BNE.S \3;dÇpassement
 MOVEQ #0,D0
\2:CMP.B #"0",(A0,D0)
  BNE.S \1
  MOVE.B #" ",(A0,D0);enlÇve les zÇros inutiles
  INC D0
 CMP D1,D0
 BNE.S \2
 BRA.S \1
\3:MOVE.B #"?",(A0,D1)
 DBRA D1,\3
\1:RTS

CONV_DECI_SIMPLE:;A0:pointeur cible chaåne ASCII, D0:valeur sur 32 bits, D1:nombre de chiffres

 MOVE D1,-(SP)
 DEC D1
 MOVE.L D0,-(SP)
\1:MOVEQ #0,D0
  MOVE (SP),D0
  DIVU #10,D0;poids fort /10
  MOVE D0,(SP);rÇsultat poids fort
  MOVE 2(SP),D0
  DIVU #10,D0;((reste * 65536) + poids faible)/10
  MOVE D0,2(SP);rÇsultat poids faible
  SWAP D0
  OR #$30,D0
  MOVE.B D0,(A0,D1)
 DBRA D1,\1
 ADDQ.L #4,SP
 MOVE (SP)+,D1
 RTS

CALLMACFUNCTION:

 MOVEM.L A0/A2/A6,-(SP)
 MOVE.L A0,A2;fonction
 SUPER 0;superviseur
 MOVE.L D0,A6;sauve SSP
 MOVE.L A2,A0
 BSR CALLMACFUNCTION2
 SUPER (A6);utilisateur
 MOVEM.L (SP)+,A0/A2/A6
 MOVE.L RETOUR,D0
 RTS

CALLMACFUNCTION2:

 MOVEM.L D1-A6,-(SP)
 MOVE.L A0,A2
 MOVE.L MODEMAC,A0
 JSR (A0)
 MOVE.L A2,-(SP)
 MOVE.L CALLMACCONTEXT,A0
 JSR (A0)
 ADDQ #4,SP
 MOVE.L MODEATARI,A0
 JSR (A0)
 MOVEM.L (SP)+,D1-A6
 MOVE.L RETOUR,D0
 RTS

GESTALT:

 MOVEM.L D0-A6,-(SP)
 MOVE.L D0,D4;selector
 MOVE.L A0,A2;response
 SUPER 0
 MOVE.L D0,D3
 MOVE.L MODEMAC,A0
 JSR (A0)
 MOVE.L A2,A0
 MOVE.L D4,D0
 MOVE.L A0,A1
 DC.W $A1AD;Gestalt
 MOVE.L A0,(A1)
 MOVE D0,D4
 MOVE.L MODEATARI,A0
 JSR (A0)
 MOVE.L D3,A0
 SUPER (A0)
 MOVE D4,D0
 MOVEM.L (SP)+,D0-A6
 RTS

ENDSOUND:

 MOVEM.L D0-A6,-(SP)
 TST CAUSE_INTER
 BEQ \2
 BMI.S \3;mfp
 BTST #0,CAUSE_INTER+1
 BEQ \1;<> lecture
 TST D_TIMER_A
 BEQ \1
 DEC COUNT_TIMER_A
 BGT \1
 MOVE D_INIT_TIMER_A,COUNT_TIMER_A
 TST D_MAC
 BEQ.S \7
 MOVE.L MODEATARI,A0
 JSR (A0);passe en mode ATARI
\7:MOVE.L VECT_TIMER_A,D0
 BEQ.S \4
 MOVE.L D0,A0
 BRA.S \5
\3:BTST #0,CAUSE_INTER+1
 BEQ.S \1;<> lecture
 TST D_IO7
 BEQ.S \1
 TST D_MAC
 BEQ.S \8
 MOVE.L MODEATARI,A0
 JSR (A0);passe en mode ATARI
\8:MOVE.L VECT_IO7,D0
 BEQ.S \4
 MOVE.L D0,A0
\5:TST PROC_TYPE
 BEQ.S \6
 CLR -(SP);format si > 68000
\6:PEA \4(PC);adresse de retour
 MOVE SR,-(SP)
 JMP (A0)
\2:TST.L INTH_ADDR
 BLE.S \1
 TST D_MAC
 BEQ.S \9
 MOVE D0,-(SP);status
 MOVE.L MODEATARI,A0
 JSR (A0);passe en mode ATARI
 MOVEQ #0,D0
 MOVE (SP)+,D0;status
\9:MOVE.L INTH_ADDR,A0
 JSR (A0)
\4:TST D_MAC
 BEQ.S \1
 MOVE.L MODEMAC,A0
 JSR (A0);repasse en mode MAC
\1:MOVEM.L (SP)+,D0-A6
 RTS

ENDSOUND_REC:

 MOVEM.L D0-A6,-(SP)
 TST CAUSE_INTER
 BEQ.S \2
 BMI.S \3;mfp
 BTST #1,CAUSE_INTER+1
 BEQ \1;<> enregistrement
 TST D_TIMER_A
 BEQ \1
 DEC COUNT_TIMER_A
 BGT \1
 MOVE D_INIT_TIMER_A,COUNT_TIMER_A
 MOVE.L MODEATARI,A0
 JSR (A0);passe en mode ATARI
 MOVE.L VECT_TIMER_A,D0
 BEQ.S \4
 MOVE.L D0,A0
 BRA.S \5
\3:BTST #1,CAUSE_INTER+1
 BEQ.S \1;<> enregistrement
 TST D_IO7
 BEQ.S \1
 MOVE.L MODEATARI,A0
 JSR (A0);passe en mode ATARI
 MOVE.L VECT_IO7,D0
 BEQ.S \4
 MOVE.L D0,A0
\5:TST PROC_TYPE
 BEQ.S \6
 CLR -(SP);format si > 68000
\6:PEA \4(PC);adresse de retour
 MOVE SR,-(SP)
 JMP (A0)
\2:TST.L INTH_ADDR_REC
 BLE.S \1
 MOVE D0,-(SP);status
 MOVE.L MODEATARI,A0
 JSR (A0);passe en mode ATARI
 MOVE.L INTH_ADDR_REC,A0
 MOVEQ #0,D0
 MOVE (SP)+,D0;status
 BSET #7,D0;enregistrement
 JSR (A0)
\4:MOVE.L MODEMAC,A0
 JSR (A0);repasse en mode MAC
\1:MOVEM.L (SP)+,D0-A6
 RTS

ISNDCMD:

 MOVE.L D2,-(SP);param2
 MOVE D1,-(SP);param1
 MOVE D0,-(SP);cmd
 MOVE.L A0,SNDCHANNELPTR
 LEA (SP),A0
 MOVE.L A0,SNDCOMMAND
 LEA SNDDOIMMEDIATE,A0
 BSR CALLMACFUNCTION2
 ADDQ #8,SP
 RTS

SNDCMD:

 MOVE.L D2,-(SP);param2
 MOVE D1,-(SP);param1
 MOVE D0,-(SP);cmd
 MOVE.L A0,SNDCHANNELPTR
 LEA (SP),A0
 MOVE.L A0,SNDCOMMAND
 LEA SNDDOCOMMAND,A0
 BSR CALLMACFUNCTION2
 ADDQ #8,SP
 RTS

MACSNDCMD:

 MOVE.L D2,-(SP);param2
 MOVE D1,-(SP);param1
 MOVE D0,-(SP);cmd
 MOVE.L A0,SNDCHANNELPTR
 MOVE.L SP,SNDCOMMAND
 BSR SNDDOCOMMAND
 MOVE.L RETOUR,D0
 BEQ.S \1
 AND #$FFFC,STATUS;arràt lecture DMA
\1:ADDQ #8,SP
 RTS

SPBVERSION:

 MOVEM.L D0-A6,-(SP)
 CLR.L -(SP)
 MOVE.L #$00000014,D0
 DC.W $A800;SPBVersion
 MOVE.L (SP)+,RETOUR;NumVersion
 MOVEM.L (SP)+,D0-A6
 RTS
 
SPBGETINDEXEDDEVICE:

 MOVEM.L D0-A6,-(SP)
 CLR -(SP)
 MOVE COUNT,-(SP)
 PEA DEVICENAME
 PEA DEVICEICONHANDLE
 MOVE.L #$05140014,D0
 DC.W $A800;SPBGetIndexedDevice
 MOVE (SP)+,D0
 EXT.L D0
 MOVE.L D0,RETOUR;err
 MOVEM.L (SP)+,D0-A6
 RTS
 
SPBOPENDEVICE:

 MOVEM.L D0-A6,-(SP)
 CLR -(SP)
 PEA NUL;deviceName: defaut
 MOVE #1,-(SP);siWritePermission
 PEA INREFNUM
 MOVE.L #$05180014,D0
 DC.W $A800
 MOVE (SP)+,D0
 EXT.L D0
 MOVE.L D0,RETOUR;err
 MOVEM.L (SP)+,D0-A6
 RTS

SPBCLOSEDEVICE:

 MOVEM.L D0-A6,-(SP)
 CLR -(SP)
 MOVE.L INREFNUM,-(SP)
 MOVE.L #$021C0014,D0
 DC.W $A800
 MOVE (SP)+,D0
 EXT.L D0
 MOVE.L D0,RETOUR;err
 MOVEM.L (SP)+,D0-A6
 RTS
 
SPBRECORD:

 MOVEM.L D0-A6,-(SP)
 CLR -(SP)
 PEA INPARAMPTR
 MOVE #-1,-(SP);asynchFlag: asynchronous
 MOVE.L #$03200014,D0
 DC.W $A800
 MOVE (SP)+,D0
 EXT.L D0
 MOVE.L D0,RETOUR;err
 MOVEM.L (SP)+,D0-A6
 RTS
 
SPBSTOPRECORDING:

 MOVEM.L D0-A6,-(SP)
 CLR -(SP)
 MOVE.L INREFNUM,-(SP)
 MOVE.L #$02300014,D0
 DC.W $A800
 MOVE (SP)+,D0
 EXT.L D0
 MOVE.L D0,RETOUR;err
 MOVEM.L (SP)+,D0-A6
 RTS
 
SPBGETDEVICEINFO:

 MOVEM.L D0-A6,-(SP)
 CLR -(SP)
 MOVE.L INREFNUM,-(SP)
 MOVE.L INFOTYPE,-(SP) 
 PEA INFODATA
 MOVE.L #$06380014,D0
 DC.W $A800
 MOVE (SP)+,D0
 EXT.L D0
 MOVE.L D0,RETOUR;err
 MOVEM.L (SP)+,D0-A6
 RTS

SPBSETDEVICEINFO:

 MOVEM.L D0-A6,-(SP)
 CLR -(SP)
 MOVE.L INREFNUM,-(SP)
 MOVE.L INFOTYPE,-(SP)
 PEA INFODATA
 MOVE.L #$063C0014,D0
 DC.W $A800
 MOVE (SP)+,D0
 EXT.L D0
 MOVE.L D0,RETOUR;err
 MOVEM.L (SP)+,D0-A6
 RTS
 
SNDSOUNDMANAGERVERSION:

 MOVEM.L D0-A6,-(SP)
 CLR.L -(SP)
 MOVE.L #$000C0008,D0;SndSoundManagerVersion
 DC.W $A800
 MOVE.L (SP)+,RETOUR;NumVersion
 MOVEM.L (SP)+,D0-A6
 RTS
 
SNDDISPOSECHANNEL:

 MOVEM.L D0-A6,-(SP)
 CLR -(SP)
 MOVE.L SNDCHANNELPTR,-(SP)
 MOVE.B #1,-(SP);quietNow
 DC.W $A801
 MOVE (SP)+,D0
 EXT.L D0
 MOVE.L D0,RETOUR;err
 MOVEM.L (SP)+,D0-A6
 RTS

SNDDOCOMMAND:

 MOVEM.L D0-A6,-(SP)
 CLR -(SP)
 MOVE.L SNDCHANNELPTR,-(SP)
 MOVE.L SNDCOMMAND,-(SP)
 MOVE #1,-(SP);noWait
 DC.W $A803
 MOVE (SP)+,D0
 EXT.L D0
 MOVE.L D0,RETOUR;err
 MOVEM.L (SP)+,D0-A6
 RTS
 
SNDDOIMMEDIATE:

 MOVEM.L D0-A6,-(SP)
 CLR -(SP)
 MOVE.L SNDCHANNELPTR,-(SP)
 MOVE.L SNDCOMMAND,-(SP)
 DC.W $A804
 MOVE (SP)+,D0
 EXT.L D0
 MOVE.L D0,RETOUR;err
 MOVEM.L (SP)+,D0-A6
 RTS

SNDNEWCHANNEL:

 MOVEM.L D0-A6,-(SP)
 CLR -(SP)
 MOVE.L SNDCHANNELPTR2,-(SP)
 MOVE #SAMPLEDSYNTH,-(SP);synth
 MOVE.L INITSND,-(SP);init
 MOVE.L SNDCALLBACKPROCPTR,-(SP);userRoutine
 DC.W $A807
 MOVE (SP)+,D0
 EXT.L D0
 MOVE.L D0,RETOUR;err
 MOVEM.L (SP)+,D0-A6
 RTS
 
DIV32_32:;divise D0 par D1 => D0 en 32 bits

 DC.L $4C410000;DIVU.L D1,D0;sans reste
 RTS

MUL32_32:;multiplie D0 par D1 => D0 en 32 bits

 DC.L $4C010000;MULU.L D1,D0
 RTS
 
MUL32_64:;multiplie D0 par D1 en 32 bits
; => rÇsultat en 64 bits: poids faible dans D0, poids fort dans D1

 MOVE.L D2,-(SP)
 DC.L $4C010402;MULU.L D1,D2:D0
 MOVE.L D2,D1;poids fort
 MOVE.L (SP)+,D2
 RTS

DIV64_32S:;divise D1-D0 en 64 bits (poids fort dans D1) par D2 en 32 bits signÇs
; => rÇsultat dans D0 en 32 bits, reste dans D1 en 32 bits

 DC.L $4C420C01;DIVS.L D2,D1:D0
 RTS
 
MUL64_TRACKS:
 
 MOVE D2,-(SP)
 MOVE NB_TRACKS,D2
 BRA.S \1
\2:ADD.L D0,D0
  ADDX.L D1,D1 
\1:DBRA D2,\2
 MOVE (SP)+,D2
 RTS

TEST_SDL_ARANYM:

 MOVEQ #0,D0
 MOVE SR,-(SP)
 OR #$700,SR
 LEA \1(PC),A0
 MOVE.L 16,A1;instruction illegale
 MOVE.L A0,16
 MOVE.L SP,A2
 CLR.L -(SP)
 DC.W $712C
 ADDQ #4,SP
 MOVEQ #-1,D0
\1:MOVE.L A1,16
 MOVE.L A2,SP
 MOVE (SP)+,SR
 MOVE D0,D_SDL
 RTS
 
SDL_OPENAUDIO:

 MOVEM.L D2-A6,-(SP)
 PEA (A0);buffer
 MOVE D3,-(SP);nb samples
 MOVE D2,-(SP);channels
 MOVE D1,-(SP);format
 MOVE.L D0,-(SP);frequency
 MOVE.L #OpenAudio,-(SP)
 DC.W $712C
 LEA 14(SP),SP
 MOVEM.L (SP)+,D2-A6
 TST.L D0
 RTS
 
SDL_CLOSEAUDIO:

 MOVEM.L D0-A6,-(SP)
 MOVE.L #CloseAudio,-(SP)
 DC.W $712C
 MOVEM.L (SP)+,D0-A6
 RTS
 
SDL_PAUSEAUDIO:

 MOVEM.L D1-A6,-(SP)
 MOVE D0,-(SP);pause_on
 MOVE.L #PauseAudio,-(SP)
 DC.W $712C
 ADDQ #2,SP
 MOVEM.L (SP)+,D1-A6
 RTS
  
SDL_AUDIOSTATUS:

 MOVEM.L D1-A6,-(SP)
 MOVE.L #AudioStatus,-(SP)
 DC.W $712C
 MOVEM.L (SP)+,D1-A6
 RTS

SDL_AUDIOVOLUME:

 MOVEM.L D1-A6,-(SP)
 MOVE D0,-(SP);volume
 MOVE.L #AudioVolume,-(SP)
 DC.W $712C
 ADDQ #2,SP
 MOVEM.L (SP)+,D1-A6
 RTS

SDL_LOCKAUDIO:

 MOVEM.L D0-A6,-(SP)
 MOVE.L #LockAudio,-(SP)
 DC.W $712C
 MOVEM.L (SP)+,D0-A6
 RTS

SDL_UNLOCKAUDIO:

 MOVEM.L D0-A6,-(SP)
 MOVE.L #UnlockAudio,-(SP)
 DC.W $712C
 MOVEM.L (SP)+,D0-A6
 RTS
 
SDL_GETAUDIOFREQ:

 MOVEM.L D1-A6,-(SP)
 MOVE.L #GetAudioFreq,-(SP)
 DC.W $712C;fonctions audio Aranym
 MOVEM.L (SP)+,D1-A6
 RTS
 
SDL_GETAUDIOLEN:

 MOVEM.L D1-A6,-(SP)
 MOVE.L #GetAudioLen,-(SP)
 DC.W $712C
 MOVEM.L (SP)+,D1-A6
 RTS

TIMER_OUTPUT_:

 MOVEM.L D0-A6,-(SP)
 MOVE.L #"hade",D0
 BSR GET_COOKIE
 SPL.B D7
 EXT.W D7
 SWAP D7;flag HADES
 MOVE INFOS_COOKIE+10,D0;slot
 SNE.B D7
 EXT.W D7;MV16/PSOUND
 SUB.L A6,A6
 SUPER 1
 TST.L D0
 BMI.S \4;superviseur
 SUPER 0
 MOVE.L D0,A6;sauve SSP
\4:LEA COMPTEUR_PSG,A0
 MOVE.L 64(SP),D0
 MOVE.L D0,(A0)+;compteur initialisÇ Ö longueur
 MOVE.L 68(SP),(A0);adresse Çchantillons
 CLR FLAG_ARRET
 MOVE SR,-(SP)
 OR #$700,SR;interdit IRQ
 BSR ARRET_PSG
 MOVE 76(SP),D0;mode: B0:mono(0)/stÇrÇo(1)
;                     B1:8 bits(0)/16 bits(1) B6:rÇpÇtition non(0)/oui(1)
 BTST #6,D0;rÇpÇtition
 BEQ.S \2
 AND #3,D0
 BSET #2,D0
\2:LEA TAB_PSG2,A0
 TST D7
 BMI.S \3;carte son HADES
 LEA TAB_PSG,A0
\3:ASL #2,D0
 MOVE.L (A0,D0),A0
 LEA SAUVE_IRQ_TIMER_A,A1
 LEA $134,A2;vecteur MFP
 TST D_TT
 BEQ.S \7
 LEA $174,A2;vecteur MFP TT
\7:MOVE.L (A2),(A1);sauve vecteur
 MOVE.L A0,(A2);nouveau vecteur
 LEA $FFFA0000,A0;port cartouche => MV16
 TST.L D7
 BPL.S \5;<> HADES
 LEA ROPOCOP_HADES,A0
 MOVE INFOS_COOKIE+10,D0;slot
 AND #3,D0
 ASL #2,D0;*4
 MOVE.L (A0,D0),A0
\5:MOVE.L A0,ADR_CARTOUCHE
 MOVE 74(SP),D0;no frÇquence
 LEA $FFFFFA01,A0;MFP
 TST D_TT
 BEQ.S \6
 LEA $FFFFFA81,A0;MFP TT
\6:MOVE.L A0,ADR_MFP
 BSR INIT_TIMER_A
 BCLR #3,$16(A0);VR automatic end of interrupt
 BSET #5,6(A0);IERA
 BSET #5,$12(A0);IMRA autorisation IRQ timer A
 BCLR #5,$E(A0);ISRA pas d'IRQ en service
 MOVE (SP)+,SR
 MOVE.L A6,D0
 BEQ.S \1
 SUPER (A6)
\1:MOVEM.L (SP)+,D0-A6
 RTS
 
INIT_TIMER_A:;D0: no freq 0-7 ou freq si >7

 MOVEM.L D0-D2/A0-A2,-(SP)
 MOVE D0,-(SP)
 MOVE.L ADR_MFP,A0;MFP 68901
 BCLR #5,$12(A0);IMRA masque timer A
 BCLR #5,6(A0);IERA inhibe timer A
 BCLR #5,$A(A0);IPRA pas pending
 BCLR #5,$E(A0);ISRA pas d'IRQ en service
 CLR.B $18(A0);TACR stoppe timer A
 MOVE (SP)+,D0
 MOVE.L #614400,D2;2.4576 MHz /4
 DIVU D0,D2;/frÇquence
 BVS.S \2
 MOVE.L D2,D1
 CLR D1
 SWAP D1
 ADD.L D1,D1;reste *2
 DIVU D0,D1
 ADD D1,D2
 CMP #255,D2
 BCS.S \1
\2:MOVEQ #-1,D2
\1:MOVE.B D2,$1E(A0);TADR valeur initiale timer A
 MOVE.B #1,$18(A0);TACR prÇdiv /4
;temps en uS = (TADR*4)/2.4576
 MOVEM.L (SP)+,D0-D2/A0-A2
 RTS

ARRET_PSG:

 MOVEM.L D0-D1/A0,-(SP)
 LEA $FFFF8800,A0;circuit sonore PSG YAMAHA
 MOVEQ #0,D0
 MOVEQ #5,D1
\1:MOVEP D0,0(A0);pÇriode canaux 1 Ö 3
  ADD #$100,D0
 DBRA D1,\1
 MOVE #$7FF,D0
 MOVEP D0,0(A0);arràt canaux & port B en sortie
 MOVEM.L (SP)+,D0-D1/A0
 RTS

FIN_IRQ_PSG:

 TST FLAG_ARRET
 BMI.S \1
 MOVEM.L D0/A0/A1,-(SP)
 MOVE SR,-(SP)
 OR #$700,SR;interdit IRQ
 MOVE.L ADR_MFP,A0;MFP 68901
 BSET #3,$16(A0);VR software end of interrupt
 BCLR #5,$12(A0);IMRA masque timer A
 BCLR #5,6(A0);IERA inhibe timer A
 BCLR #5,$A(A0);IPRA pas pending
 BCLR #5,$E(A0);ISRA pas d'IRQ en service
 CLR.B $18(A0);TACR stoppe timer A
 MOVE.L SAUVE_IRQ_TIMER_A,A0
 LEA $134,A1;vecteur MFP
 TST D_TT
 BEQ.S \3
 LEA $174,A1;vecteur MFP TT
\3:MOVE.L A0,(A1)
 BSR ARRET_PSG
 MOVE #-1,FLAG_ARRET;arràt ou fin
 BTST #0,STATUS+1;lecture DMA
 BEQ.S \2;arràt dÇja demandÇ
 AND #$FFFC,STATUS;arràt lecture DMA
 MOVEQ #0,D0
 BSR ENDSOUND
\2:MOVE (SP)+,SR
 MOVEM.L (SP)+,D0/A0/A1
\1:RTS

IRQ_PSG2_STEREO16_REP:

 MOVEM.L D0/D1/A0,-(SP)
 LEA COMPTEUR_PSG(PC),A0
 SUBQ.L #4,(A0)+;compteur
 BPL.S I15
 BRA I10;swap

IRQ_PSG2_MONO16_REP:

 MOVEM.L D0/D1/A0,-(SP)
 LEA COMPTEUR_PSG(PC),A0
 SUBQ.L #2,(A0)+;compteur
 BPL.S I17
 BRA I10;swap

IRQ_PSG2_STEREO16:

 MOVEM.L D0/D1/A0,-(SP)
 LEA COMPTEUR_PSG(PC),A0
 SUBQ.L #4,(A0)+;compteur
 BPL.S I15
 BSR FIN_IRQ_PSG
 BRA I6
I15:ADDQ.L #4,(A0);adresse
 MOVE 4(A0),-(SP);volume
 MOVE.L (A0),A0;adresse
 MOVE -(A0),D0;Çchantillons
 EXT.L D0
 MOVE -(A0),D1
 EXT.L D1
 ADD.L D1,D0
 ASR.L #4,D0;moyenne sur 12 bits
 MOVE (SP)+,D1;volume
 ASR D1,D0
 AND #(4095*2),D0
 BCHG #12,D0;non signÇ
 MOVE.L ADR_CARTOUCHE,A0;MV16 utilisÇe sur 12 bits
 MOVE.B (A0,D0),D1
;     5  4  3  2  1  0  9  8  7  6  5  4  3  2  1  0
;----------------------------------------------------
;$FA  #  #  #  D  D  D  D  D  D  D  D  C  C  C  C  #
;----------------------------------------------------
;     |     |  |                    |  |        |  
;     < N/A >  < volume data 8-bits >  < channel>     
 LEA $FFFF8800,A0;PSG YAMAHA port //
 MOVE.B #15,(A0);port B
 LSR #5,D0;8 bits
 MOVE.B D0,2(A0)
 MOVEM.L (SP)+,D0/D1/A0
 RTE

IRQ_PSG2_MONO16:

 MOVEM.L D0/D1/A0,-(SP)
 LEA COMPTEUR_PSG(PC),A0
 SUBQ.L #2,(A0)+;compteur
 BPL.S I17
 BSR FIN_IRQ_PSG
 BRA I6
I17:ADDQ.L #2,(A0);adresse
 MOVE 4(A0),D1;volume
 MOVE.L (A0),A0;adresse
 MOVE -(A0),D0;Çchantillon
 ASR #3,D0;sur 12 bits
 ASR D1,D0;volume
 AND #(4095*2),D0
 BCHG #12,D0;non signÇ
 MOVE.L ADR_CARTOUCHE,A0;MV16 utilisÇe sur 12 bits
 MOVE.B (A0,D0),D1
;     5  4  3  2  1  0  9  8  7  6  5  4  3  2  1  0
;----------------------------------------------------
;$FA  #  #  #  D  D  D  D  D  D  D  D  C  C  C  C  #
;----------------------------------------------------
;     |     |  |                    |  |        |  
;     < N/A >  < volume data 8-bits >  < channel>     
 LEA $FFFF8800,A0;PSG YAMAHA port //
 MOVE.B #15,(A0);port B
 LSR #5,D0;8 bits
 MOVE.B D0,2(A0)
 MOVEM.L (SP)+,D0/D1/A0
 RTE

IRQ_PSG2_STEREO_REP:

 MOVEM.L D0/D1/A0,-(SP)
 LEA COMPTEUR_PSG(PC),A0
 SUBQ.L #2,(A0)+;compteur
 BPL.S I9
 BRA.S I10;swap

IRQ_PSG2_MONO_REP:

 MOVEM.L D0/D1/A0,-(SP)
 LEA COMPTEUR_PSG(PC),A0
 SUBQ.L #1,(A0)+;compteur
 BPL.S I7
I10:BSR SWAP_ADR
 MOVE.L ADR_DEBUT,D0
 MOVE.L D0,(A0);adresse
 NEG.L D0
 ADD.L ADR_FIN,D0
 MOVE.L D0,-(A0);compteur
 BRA.S I6

IRQ_PSG2_STEREO:

 MOVEM.L D0/D1/A0,-(SP)
 LEA COMPTEUR_PSG(PC),A0
 SUBQ.L #2,(A0)+;compteur
 BPL.S I9
 BSR FIN_IRQ_PSG
 BRA.S I6
I9:ADDQ.L #2,(A0);adresse
 MOVE 4(A0),-(SP);volume
 MOVE.L (A0),A0;adresse
 MOVE.B -(A0),D0;Çchantillons
 EXT.W D0
 MOVE.B -(A0),D1
 EXT.W D1
 ADD D1,D0
 ASR #1,D0;moyenne
 AND #255,D0;8 bits
 MOVE (SP)+,D1;volume
 BRA.S I8

IRQ_PSG2_MONO:

 MOVEM.L D0/D1/A0,-(SP)
 LEA COMPTEUR_PSG(PC),A0
 SUBQ.L #1,(A0)+;compteur
 BPL.S I7
 BSR FIN_IRQ_PSG
 BRA.S I6
I7:ADDQ.L #1,(A0);adresse
 MOVE 4(A0),D1;volume
 MOVE.L (A0),A0;adresse
 MOVEQ #0,D0
 MOVE.B -(A0),D0;Çchantillon
I8:ASR.B D1,D0;volume
 BCHG #7,D0;non signÇ
 LEA $FFFF8800,A0;PSG YAMAHA port //
 MOVE.B #15,(A0);port B
 MOVE.B D0,2(A0)
 MOVE.L ADR_CARTOUCHE,A0;MV16 voie 1
 ASL #5,D0
 MOVE.B (A0,D0),D0
;     5  4  3  2  1  0  9  8  7  6  5  4  3  2  1  0
;----------------------------------------------------
;$FA  #  #  #  D  D  D  D  D  D  D  D  C  C  C  C  #
;----------------------------------------------------
;     |     |  |                    |  |        |  
;     < N/A >  < volume data 8-bits >  < channel>      
I6:MOVEM.L (SP)+,D0/D1/A0
 RTE

IRQ_PSG_STEREO16_REP:

 MOVEM.L D0/D1/A0/A1,-(SP)
 LEA COMPTEUR_PSG(PC),A0
 SUBQ.L #4,(A0)+;compteur
 BPL.S I14
 BRA I5;swap

IRQ_PSG_MONO16_REP:

 MOVEM.L D0/D1/A0/A1,-(SP)
 LEA COMPTEUR_PSG(PC),A0
 SUBQ.L #2,(A0)+;compteur
 BPL.S I16
 BRA I5;swap

IRQ_PSG_STEREO16:

 MOVEM.L D0/D1/A0/A1,-(SP)
 LEA COMPTEUR_PSG(PC),A0
 SUBQ.L #4,(A0)+;compteur
 BPL.S I14
 BSR FIN_IRQ_PSG
 BRA I1
I14:ADDQ.L #4,(A0);adresse
 MOVE.L (A0)+,A1
 MOVE -(A1),D0;Çchantillons
 EXT.L D0
 MOVE -(A1),D1
 EXT.L D1
 ADD.L D1,D0
 MOVEQ #9,D1
 ASR.L D1,D0;moyenne
 AND #255,D0;8 bits
 MOVE (A0),D1;volume
 BRA I3

IRQ_PSG_MONO16:

 MOVEM.L D0/D1/A0/A1,-(SP)
 LEA COMPTEUR_PSG(PC),A0
 SUBQ.L #2,(A0)+;compteur
 BPL.S I16
 BSR FIN_IRQ_PSG
 BRA I1
I16:ADDQ.L #2,(A0);adresse
 MOVE.L (A0)+,A1
 MOVE (A0),D1;volume
 MOVE -(A1),D0;Çchantillon
 BRA.S I3

IRQ_PSG_STEREO_REP:

 MOVEM.L D0/D1/A0/A1,-(SP)
 LEA COMPTEUR_PSG(PC),A0
 SUBQ.L #2,(A0)+;compteur
 BPL.S I4
 BRA.S I5;swap

IRQ_PSG_MONO_REP:

 MOVEM.L D0/D1/A0/A1,-(SP)
 LEA COMPTEUR_PSG(PC),A0
 SUBQ.L #1,(A0)+;compteur
 BPL.S I2
I5:BSR SWAP_ADR
 MOVE.L ADR_DEBUT,D0
 MOVE.L D0,(A0);adresse
 NEG.L D0
 ADD.L ADR_FIN,D0
 MOVE.L D0,-(A0);compteur
 BRA.S I1

IRQ_PSG_STEREO:

 MOVEM.L D0/D1/A0/A1,-(SP)
 LEA COMPTEUR_PSG(PC),A0
 SUBQ.L #2,(A0)+;compteur
 BPL.S I4
 BSR FIN_IRQ_PSG
 BRA.S I1
I4:ADDQ.L #2,(A0);adresse
 MOVE.L (A0)+,A1
 MOVE.B -(A1),D0;Çchantillons
 EXT.W D0
 MOVE.B -(A1),D0
 EXT.W D1
 ADD D1,D0
 ASR #1,D0;moyenne
 AND #255,D0;8 bits
 MOVE (A0),D1;volume
 BRA.S I3

IRQ_PSG_MONO:

 MOVEM.L D0/D1/A0/A1,-(SP)
 LEA COMPTEUR_PSG(PC),A0
 SUBQ.L #1,(A0)+;compteur
 BPL.S I2
 BSR FIN_IRQ_PSG
 BRA.S I1
I2:ADDQ.L #1,(A0);adresse
 MOVE.L (A0)+,A1
 MOVE (A0),D1;volume
 MOVEQ #0,D0
 MOVE.B -(A1),D0;Çchantillon
I3:ASR.B D1,D0
 ASL #2,D0
 LEA $FFFF8800,A0;PSG YAMAHA
 LEA TAB_VOL+1(PC,D0),A1
 MOVEQ #8,D0
 MOVE.B D0,(A0);volume 1
 MOVE.B (A1)+,2(A0);MOVEP inexistant sur 060
 INC D0
 MOVE.B D0,(A0);volume 2
 MOVE.B (A1)+,2(A0)
 INC D0
 MOVE.B D0,(A0);volume 3
 MOVE.B (A1),2(A0)
I1:MOVEM.L (SP)+,D0/D1/A0/A1
 RTE

 DATA

COMPTEUR_PSG:DC.L 0;
ADRESSE:DC.L 0     ; ne pas changer l'ordre
VOLUME:DC.W 0      ;
 
TAB_VOL:

 DC.L $C0B09,$C0B09,$D0808,$B0B0B,$D0905,$C0B08,$D0902,$D0806,$C0B07,$D0707,$C0B06,$C0A09,$B0B0A,$C0B02,$C0B00,$C0A08
 DC.L $D0604,$D0505,$D0504,$C0909,$D0403,$B0B09,$C0A05,$B0A0A,$C0908,$B0B08,$C0A00,$C0A00,$C0907,$B0B07,$C0906,$B0B06
 DC.L $B0A09,$B0B05,$A0A0A,$B0B02,$B0A08,$C0707,$C0804,$C0706,$B0909,$C0606,$A0A09,$C0703,$B0A05,$B0908,$B0A03,$A0A08
 DC.L $B0A00,$B0907,$B0808,$A0A07,$A0909,$C0101,$A0A06,$B0807,$A0A05,$A0908,$A0A02,$A0A01,$A0A00,$90909,$A0808,$B0801
 DC.L $A0906,$B0704,$A0905,$90908,$A0903,$A0806,$A0900,$90907,$90808,$A0804,$90906,$A0801,$90905,$90807,$80808,$90902
 DC.L $90806,$90900,$90707,$80807,$90706,$90802,$80806,$90606,$80707,$80804,$80706,$80802,$70707,$80606,$80704,$70706
 DC.L $80605,$80604,$70606,$70704,$80504,$60606,$70604,$70505,$60605,$60604,$60505,$60602,$60504,$50505,$60502,$50504
 DC.L $50404,$50502,$40404,$40403,$40402,$40303,$30303,$30302,$30202,$20202,$20201,$10101,$20100,$10100,$10000,$00000

 DC.L $E0D0C,$F0300,$F0300,$F0300,$F0300,$F0300,$F0300,$E0D0B,$E0D0B,$E0D0B,$E0D0B,$E0D0B,$E0D0B,$E0D0B,$E0D0A,$E0D0A
 DC.L $E0D0A,$E0D0A,$E0C0C,$E0D00,$D0D0D,$D0D0D,$D0D0D,$D0D0D,$D0D0D,$D0D0D,$E0C0B,$E0C0B,$E0C0B,$E0C0B,$E0C0B,$E0C0B
 DC.L $E0C0B,$E0C0B,$E0C0A,$E0C0A,$E0C0A,$E0C0A,$D0D0C,$D0D0C,$E0C09,$E0C09,$E0C05,$E0C00,$E0C00,$E0B0B,$E0B0B,$E0B0B
 DC.L $E0B0B,$E0B0A,$E0B0A,$E0B0A,$D0D0B,$D0D0B,$D0D0B,$E0B09,$E0B09,$E0B09,$D0C0C,$D0D0A,$E0B07,$E0B00,$E0B00,$D0D09
 DC.L $D0D09,$E0A09,$D0D08,$D0D07,$D0D04,$D0D00,$E0A04,$E0909,$E0909,$D0C0B,$E0908,$E0908,$E0907,$E0808,$E0901,$C0C0C
 DC.L $D0C0A,$E0806,$E0707,$E0800,$E0705,$E0606,$D0C09,$E0505,$E0404,$D0C08,$D0B0B,$E0000,$D0C06,$D0C05,$D0C02,$C0C0B
 DC.L $C0C0B,$D0B0A,$D0B0A,$D0B0A,$D0B0A,$C0C0A,$C0C0A,$C0C0A,$D0B09,$D0B09,$D0A0A,$D0A0A,$D0A0A,$C0C09,$C0C09,$C0C09
 DC.L $D0B06,$C0B0B,$C0C08,$D0B00,$D0B00,$C0C07,$C0C06,$C0C05,$C0C03,$C0C01,$C0B0A,$D0A05,$D0A04,$D0A02,$D0908,$D0908

TAB_PSG:

 DC.L IRQ_PSG_MONO,IRQ_PSG_STEREO,IRQ_PSG_MONO16,IRQ_PSG_STEREO16
 DC.L IRQ_PSG_MONO_REP,IRQ_PSG_STEREO_REP,IRQ_PSG_MONO16_REP,IRQ_PSG_STEREO16_REP
 
TAB_PSG2:

 DC.L IRQ_PSG2_MONO,IRQ_PSG2_STEREO,IRQ_PSG2_MONO16,IRQ_PSG2_STEREO16
 DC.L IRQ_PSG2_MONO_REP,IRQ_PSG2_STEREO_REP,IRQ_PSG2_MONO16_REP,IRQ_PSG2_STEREO16_REP

ROPOCOP_HADES:DC.L $FFFA0000,$FF7A0000,$FF7C0000,$FF7E0000

STRUCT_MCSN:

 DC.W VERSION
 DC.W 30
 DC.W 2;play FALCON
 DC.W 2;record FALCON
 DC.W 0;short dsp; Is the DSP there?
 DC.W 1;Playing: Interrupt possible with frame-end?
 DC.W 1;Recording: Interrupt possible with frame-end?
 DC.L 22579200,0,0,0;rÇserve

TAB_NUM_FREQ_FALCON:
;frÇquences internes
 DC.W 49170,33800,24585,20770,16940,16940,12292,12292,9834,9834,8195; 25,175 MHz
;frÇquences via horloge externe
 DC.W 44100,29400,22050,17640,14700,14700,11025,11025,8820,8820,7350; 22,5792 MHz
 DC.W 48000,32000,24000,19200,16000,16000,12000,12000,9600,9600,8000; 24,576 MHz
TAB_NUM_FREQ_STE:DC.W 6250,12500,25000,50000
VOLUMES:DC.W 255,255
GAINS:DC.W 0,0

INFOS_COOKIE:

 DC.W VERSION
 DC.W 0;NB_SOURCES
 DC.W 0;INSOURCE
 DC.L TAB_SOURCENAME
 DC.W 0;SLOT

NUL:DC.B 0
MESS1:DC.B 13,10,10,27,"p MagiCMac Sound Driver XBIOS v",0
MESS2:DC.B " ",27,"q installed",13,10,"Didier MEQUIGNON  April 2002",0
MESS3:DC.B 13,10,"MacOS Sound Manager v",0
MESS4:DC.B 13,10,"MacOS Sound Input Manager v",0
MESS5:DC.B 13,10,"Input Devices:",0 
MESS6:DC.B 13,10,"Current Input Device: ",0
MESS7:DC.B 13,10,"Input Sources:",0
MESS8:DC.B 13,10,"Aranym detected, use SDL Sound",0
ERR1:DC.B 13,10,"Channel XBIOS cannot be open !",0
ERR2:DC.B 13,10,"MAC without sound !",0
ERR3:DC.B 13,10,"Impossible to open input device !",0
ERR4:DC.B 13,10,"No input source...",0
ERR5:DC.B 13,10,"No stereo input...",0
ERR6:DC.B 13,10,"No 16-bits input...",0
ERR7:DC.B 13,10,"No input device !",0
ERR8:DC.B 13,10,"Hardware cannot play stereo sounds !",0
ERR9:DC.B 13,10,"Hardware cannot handle 16-bits sounds !",0
ERR10:DC.B 13,10,"Hardware cannot record stereo sounds !",0

DEBUG1:
 IFNE 0,DEBUG
 DC.B 13,10,"Driver non installÇ",0
 ENDIF
DEBUG2:
 IFNE 0,DEBUG
 DC.B 13,10,"Cookie _SND initialisÇ",0
 ENDIF
DEBUG3:
 IFNE 0,DEBUG
 DC.B 13,10,"Attributs MacOS testÇs",13,10," GestaltSoundAttr:",0
 ENDIF
DEBUG4:
 IFNE 0,DEBUG
 DC.B 13,10,"CrÇation canal audio",0
 ENDIF
DEBUG5:
 IFNE 0,DEBUG
 DC.B 13,10,"Cookie MgSn crÇÇ",0
 ENDIF
DEBUG6:
 IFNE 0,DEBUG
 DC.B 13,10,"LOCKSND     ",0
 ENDIF
DEBUG7:
 IFNE 0,DEBUG
 DC.B 13,10,"UNLOCKSND   ",0
 ENDIF
DEBUG8:
 IFNE 0,DEBUG
 DC.B 13,10,"SOUNDCMD    ",0
 ENDIF
DEBUG9:
 IFNE 0,DEBUG
 DC.B 13,10,"SETBUFFER   ",0
 ENDIF
DEBUG10:
 IFNE 0,DEBUG
 DC.B 13,10,"BUFFOPER    ",0
 ENDIF
DEBUG11:
 IFNE 0,DEBUG
 DC.B 13,10,"SETMODE     ",0
 ENDIF
DEBUG12:
 IFNE 0,DEBUG
 DC.B 13,10,"DEVCONNECT  ",0
 ENDIF
DEBUG13:
 IFNE 0,DEBUG
 DC.B 13,10,"SNDSTATUS   ",0
 ENDIF
DEBUG14:
 IFNE 0,DEBUG
 DC.B 13,10,"BUFFPTR     ",0
 ENDIF
DEBUG15:
 IFNE 0,DEBUG
 DC.B 13,10,"SETINTERRUPT",0
 ENDIF
DEBUG16:
 IFNE 0,DEBUG
 DC.B 13,10,"GPIO        ",0
 ENDIF
DEBUG17:
 IFNE 0,DEBUG
 DC.B 13,10,"SETTRACKS   ",0
 ENDIF
DEBUG18:
 IFNE 0,DEBUG
 DC.B 13,10,"SETMONTRACKS",0
 ENDIF
DEBUG19:
 IFNE 0,DEBUG
 DC.B 13,10,"JDISINT     ",0
 ENDIF
DEBUG20:
 IFNE 0,DEBUG
 DC.B 13,10,"JENABINT    ",0
 ENDIF
DEBUG21:
 IFNE 0,DEBUG
 DC.B 13,10,"XBTIMER     ",0
 ENDIF
DEBUG22:
 IFNE 0,DEBUG
 DC.B 13,10,"MFPINT      ",0
 ENDIF
DEBUG23:
 IFNE 0,DEBUG
 DC.B 13,10,"SETEXEC     ",0
 ENDIF
DEBUG50:
 IFNE 0,DEBUG
 DC.B 13,10," SndSoundManagerVersion",0
 ENDIF 
DEBUG100:
 IFNE 0,DEBUG
 DC.B 13,10," flushcmd",0
 ENDIF
DEBUG101:
 IFNE 0,DEBUG
 DC.B 13,10," quietcmd",0
 ENDIF
DEBUG102:
 IFNE 0,DEBUG
 DC.B 13,10," buffercmd",0
 ENDIF
DEBUG103:
 IFNE 0,DEBUG
 DC.B 13,10," callbackcmd",0
 ENDIF
DEBUG104:
 IFNE 0,DEBUG
 DC.B 13,10," volumecmd",0
 ENDIF
DEBUG110:
 IFNE 0,DEBUG
 DC.B 13,10," fin BUFFOPER",0
 ENDIF
DEBUG111:
 IFNE 0,DEBUG
 DC.B 13,10," status BUFFOPER:",0
 ENDIF
DEBUG112:
 IFNE 0,DEBUG
 DC.B 13,10," adresse dÇbut DMA:",0
 ENDIF
DEBUG113:
 IFNE 0,DEBUG
 DC.B 13,10," adresse fin DMA:",0
 ENDIF
DEBUG114:
 IFNE 0,DEBUG
 DC.B 13,10," frÇquence:",0
 ENDIF
DEBUG115:
 IFNE 0,DEBUG
 DC.B 13,10," octets/Çchantillon:",0
 ENDIF
DEBUG116:
 IFNE 0,DEBUG
 DC.B " status DMA:",0
 ENDIF
DEBUG117:
 IFNE 0,DEBUG
 DC.B 13,10," compteur DMA:",0
 ENDIF
DEBUG120:
 IFNE 0,DEBUG
 DC.B 13,10," SPBGetDeviceInfo",0
 ENDIF  
DEBUG121:
 IFNE 0,DEBUG
 DC.B 13,10," SPBSetDeviceInfo",0
 ENDIF
DEBUG122:
 IFNE 0,DEBUG
 DC.B 13,10," SPBRecord",0
 ENDIF    
DEBUG123:
 IFNE 0,DEBUG
 DC.B 13,10," SPBStopRecording",0
 ENDIF  
DEBUG124:
 IFNE 0,DEBUG
 DC.B 13,10," SPBGetIndexedDevice",0
 ENDIF
DEBUG125:
 IFNE 0,DEBUG
 DC.B 13,10," SPBOpenDevice",0
 ENDIF  
DEBUG126:
 IFNE 0,DEBUG
 DC.B 13,10," SPBVersion",0
 ENDIF  
DEBUG127:
 IFNE 0,DEBUG
 DC.B 13,10," SPBCloseDevice",0
 ENDIF 
DEBUG150:
 IFNE 0,DEBUG
 DC.B 13,10," siInputSource",0
 ENDIF  
DEBUG151:
 IFNE 0,DEBUG
 DC.B 13,10," siAsync",0
 ENDIF  
DEBUG152:
 IFNE 0,DEBUG
 DC.B 13,10," siNumberChannels",0
 ENDIF  
DEBUG153:
 IFNE 0,DEBUG
 DC.B 13,10," siSampleSize",0
 ENDIF  
DEBUG154:
 IFNE 0,DEBUG
 DC.B 13,10," siSampleRateAvailable",0
 ENDIF  
DEBUG155:
 IFNE 0,DEBUG
 DC.B 13,10," siCompressionType",0
 ENDIF  
DEBUG156:
 IFNE 0,DEBUG
 DC.B 13,10," siTwosComplementOnOff",0
 ENDIF  
DEBUG157:
 IFNE 0,DEBUG
 DC.B 13,10," siPlayThruOnOff",0
 ENDIF  
DEBUG158:
 IFNE 0,DEBUG
 DC.B 13,10," siStereoInputGain",0
 ENDIF  
DEBUG159:
 IFNE 0,DEBUG
 DC.B 13,10," siInputSourceNames",0
 ENDIF  
DEBUG160:
 IFNE 0,DEBUG
 DC.B 13,10," siDeviceName",0
 ENDIF  

TAB_CAR_MAC:DC.B "ÇéäèÖ‡É‚ÇÈàÍäËâÎåÓãÔó˘ñ˚ìÙîˆáÁ",0

 ALIGN.W
 BSS
D_MAC:DS.W 1
D_TT:DS.W 1
D_SDL:DS.W 1
D_OPEN_SDL:DS.W 1
D_INTEL_SDL:DS.W 1
FLAG_ARRET:DS.W 1
SAUVE_IRQ_TIMER_A:DS.L 1
ADR_CARTOUCHE:DS.L 1
ADR_MFP:DS.L 1
SAUVE_RESVALID:DS.L 1
RETOUR:DS.L 1
COUNT:DS.W 1
HORLOGE:DS.W 1
HORLOGE2:DS.W 1
PRESCALE:DS.W 1
PRESCALE_STE:DS.W 1
STATUS:DS.W 1
ADR:DS.L 1
ADR_DEBUT:DS.L 1
NEW_ADR_DEBUT:DS.L 1
ADR_FIN:DS.L 1
NEW_ADR_FIN:DS.L 1
ADR_REC:DS.L 1
ADR_DEBUT_REC:DS.L 1
NEW_ADR_DEBUT_REC:DS.L 1
ADR_FIN_REC:DS.L 1
NEW_ADR_FIN_REC:DS.L 1
ADR_SUITE:DS.L 1
COMPT_BLOC:DS.L 1
COMPT_BLOC_REC:DS.L 1
FREQUENCE:DS.L 1
FREQ_LECT:DS.L 1
OCT_ECH:DS.W 1
NB_VOIES:DS.W 1
NB_TRACKS:DS.W 1
NB_TRACKS_REC:DS.W 1
NB_INDEV:DS.W 1
NB_SOURCES:DS.W 1
INSOURCE:DS.W 1
NB_FREQ_REC:DS.W 1
FREQ_REC:DS.L 1
TAB_FREQ_REC:DS.L 16
TRACK:DS.W 1
INTH_ADDR:DS.L 1
INTH_ADDR_REC:DS.L 1
CAUSE_INTER:DS.W 1
COUNT_TIMER_A:DS.W 1
D_INIT_TIMER_A:DS.W 1
D_TIMER_A:DS.W 1
D_IO7:DS.W 1
D_SND_LOCK:DS.W 1
D_SETBUFF:DS.W 1
D_SETBUFF_REC:DS.W 1
MODEMAC:DS.L 1
MODEATARI:DS.L 1
GETBASEMODE:DS.L 1
CALLMACCONTEXT:DS.L 1
ATARIZEROPAGE:DS.L 1
INPARAMPTR:DS.L 9
INREFNUM:DS.L 1
INFOTYPE:DS.L 1
DEVICEICONHANDLE:DS.L 1
INFODATA:DS.L 8
RECORDINGSTATUS:DS.W 1
METERLEVEL:DS.W 1
TOTALSAMPLESTORECORD:DS.L 1
NUMBEROFSAMPLESRECORDED:DS.L 1
TOTALMSECSTORECORD:DS.L 1
NUMBEROFSECSRECORDED:DS.L 1
SNDCHANNELPTR:DS.L 1
SNDCHANNELPTR2:DS.L 1
INITSND:DS.L 1
SNDCOMMAND:DS.L 1
SNDCALLBACKPROCPTR:DS.L 1
INFOS_SON_MAC:DS.L 1
CHANNEL:DS.L 9+(128*2);2 paramätres par SndCommand
DEVICENAME:DS.B 256
TAB_BUFFERS:DS.B (TAILLE_BUFFER+TAILLE_SOUND_HEADER)*(NB_BUFFERS+1)
TAB_SOURCENAME:DS.B (256*MAX_TAB)+1
FIN:

 END
                                           