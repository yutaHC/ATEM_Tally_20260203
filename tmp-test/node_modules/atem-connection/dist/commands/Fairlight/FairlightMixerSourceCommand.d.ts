/// <reference types="node" />
import { FairlightAudioSourceProperties } from '../../state/fairlight';
import { AtemState } from '../../state';
import { DeserializedCommand, WritableCommand } from '../CommandBase';
import { OmitReadonly } from '../../lib/types';
import { BigInteger } from 'big-integer';
export declare class FairlightMixerSourceDeleteCommand extends DeserializedCommand<{}> {
    static readonly rawName = "FASD";
    readonly index: number;
    readonly source: BigInteger;
    constructor(index: number, source: BigInteger);
    static deserialize(rawCommand: Buffer): FairlightMixerSourceDeleteCommand;
    applyToState(state: AtemState): string | string[];
}
export declare class FairlightMixerSourceCommand extends WritableCommand<OmitReadonly<FairlightAudioSourceProperties>> {
    static MaskFlags: {
        framesDelay: number;
        gain: number;
        stereoSimulation: number;
        equalizerEnabled: number;
        equalizerGain: number;
        makeUpGain: number;
        balance: number;
        faderGain: number;
        mixOption: number;
    };
    static readonly rawName = "CFSP";
    readonly index: number;
    readonly source: BigInteger;
    constructor(index: number, source: BigInteger);
    serialize(): Buffer;
}
export declare class FairlightMixerSourceUpdateCommand extends DeserializedCommand<FairlightAudioSourceProperties> {
    static readonly rawName = "FASP";
    readonly index: number;
    readonly source: BigInteger;
    constructor(index: number, source: BigInteger, props: FairlightAudioSourceProperties);
    static deserialize(rawCommand: Buffer): FairlightMixerSourceUpdateCommand;
    applyToState(state: AtemState): string;
}
