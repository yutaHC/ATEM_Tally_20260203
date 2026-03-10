import { AudioSourceType, ExternalPortType, AudioMixOption } from '../enums';
export declare type AudioChannel = ClassicAudioChannel;
export declare type AudioMasterChannel = ClassicAudioMasterChannel;
export declare type AtemAudioState = AtemClassicAudioState;
export interface ClassicAudioChannel {
    readonly sourceType: AudioSourceType;
    portType: ExternalPortType;
    mixOption: AudioMixOption;
    /** Gain in decibel, -Infinity to +6dB */
    gain: number;
    /** Balance, -50 to +50 */
    balance: number;
    readonly supportsRcaToXlrEnabled: boolean;
    rcaToXlrEnabled: boolean;
}
export interface ClassicAudioMasterChannel {
    /** Gain in decibel, -Infinity to +6dB */
    gain: number;
    /** Balance, -50 to +50 */
    balance: number;
    followFadeToBlack: boolean;
}
export interface AtemClassicAudioState {
    readonly numberOfChannels?: number;
    readonly hasMonitor?: boolean;
    channels: {
        [channelId: number]: ClassicAudioChannel | undefined;
    };
    master?: ClassicAudioMasterChannel;
}
