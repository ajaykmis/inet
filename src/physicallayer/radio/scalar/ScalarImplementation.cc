//
// Copyright (C) 2013 OpenSim Ltd.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program; if not, see <http://www.gnu.org/licenses/>.
//

#include "ScalarImplementation.h"
#include "IRadioChannel.h"

Define_Module(ScalarRadioSignalFreeSpaceAttenuation);
Define_Module(ScalarRadioBackgroundNoise);
Define_Module(ScalarSNRRadioDecider);
Define_Module(ScalarRadioSignalModulator);

void ScalarRadioSignalTransmission::printToStream(std::ostream &stream) const
{
    RadioSignalTransmissionBase::printToStream(stream);
    stream << ", power = " << power << ", carrier frequency = " << carrierFrequency << ", bandwidth = " << bandwidth;
}

void ScalarRadioSignalReception::printToStream(std::ostream &stream) const
{
    RadioSignalReceptionBase::printToStream(stream);
    stream << ", power = " << power << ", carrier frequency = " << carrierFrequency << ", bandwidth = " << bandwidth;
}

double ScalarRadioSignalNoise::computeMaximumPower(simtime_t startTime, simtime_t endTime) const
{
    double noisePower = 0;
    double maximumNoisePower = 0;
    for (std::map<simtime_t, double>::const_iterator it = powerChanges->begin(); it != powerChanges->end(); it++)
    {
        noisePower += it->second;
        if (noisePower > maximumNoisePower && startTime <= it->first && it->first <= endTime)
            maximumNoisePower = noisePower;
    }
    return maximumNoisePower;
}


const IRadioSignalReception *ScalarRadioSignalAttenuationBase::computeReception(const IRadio *receiverRadio, const IRadioSignalTransmission *transmission) const
{
    const IRadioChannel *radioChannel = receiverRadio->getXRadioChannel();
    const IRadio *transmitterRadio = transmission->getRadio();
    const IRadioAntenna *receiverAntenna = receiverRadio->getReceiverAntenna();
    const IRadioAntenna *transmitterAntenna = transmitterRadio->getTransmitterAntenna();
    const ScalarRadioSignalTransmission *scalarTransmission = check_and_cast<const ScalarRadioSignalTransmission *>(transmission);
    IMobility *receiverAntennaMobility = receiverAntenna->getMobility();
    simtime_t receptionStartTime = radioChannel->computeTransmissionStartArrivalTime(transmission, receiverAntennaMobility);
    simtime_t receptionEndTime = radioChannel->computeTransmissionEndArrivalTime(transmission, receiverAntennaMobility);
    Coord receptionStartPosition = receiverAntennaMobility->getPosition(receptionStartTime);
    Coord receptionEndPosition = receiverAntennaMobility->getPosition(receptionEndTime);
    // TODO: revise
    Coord direction = receptionStartPosition - transmission->getStartPosition();
    double transmitterAntennaGain = transmitterAntenna->getGain(direction);
    double receiverAntennaGain = receiverAntenna->getGain(direction);
    double attenuationFactor = check_and_cast<const ScalarRadioSignalLoss *>(computeLoss(receiverRadio, transmission, receptionStartTime, receptionEndTime, receptionStartPosition, receptionEndPosition))->getFactor();
    double transmissionPower = scalarTransmission->getPower();
    double receptionPower = transmitterAntennaGain * receiverAntennaGain * attenuationFactor * transmissionPower;
    return new ScalarRadioSignalReception(receiverRadio, transmission, receptionStartTime, receptionEndTime, receptionStartPosition, receptionEndPosition, receptionPower, scalarTransmission->getCarrierFrequency(), scalarTransmission->getBandwidth());
}

const IRadioSignalLoss *ScalarRadioSignalFreeSpaceAttenuation::computeLoss(const IRadio *radio, const IRadioSignalTransmission *transmission, simtime_t startTime, simtime_t endTime, Coord startPosition, Coord endPosition) const
{
    const ScalarRadioSignalTransmission *scalarTransmission = check_and_cast<const ScalarRadioSignalTransmission *>(transmission);
    double pathLoss = computePathLoss(transmission, startTime, endTime, startPosition, endPosition, scalarTransmission->getCarrierFrequency());
    return new ScalarRadioSignalLoss(pathLoss);
}

const IRadioSignalLoss *ScalarRadioSignalCompoundAttenuation::computeLoss(const IRadio *radio, const IRadioSignalTransmission *transmission, simtime_t startTime, simtime_t endTime, Coord startPosition, Coord endPosition) const
{
    double totalLoss;
    for (std::vector<const IRadioSignalAttenuation *>::const_iterator it = elements->begin(); it != elements->end(); it++)
    {
        const IRadioSignalAttenuation *element = *it;
        const ScalarRadioSignalLoss *scalarLoss = check_and_cast<const ScalarRadioSignalLoss *>(element->computeLoss(radio, transmission, startTime, endTime, startPosition, endPosition));
        totalLoss *= scalarLoss->getFactor();
    }
    return new ScalarRadioSignalLoss(totalLoss);
}

void ScalarRadioBackgroundNoise::initialize(int stage)
{
    if (stage == INITSTAGE_LOCAL)
    {
        power = par("power");
    }
}

const IRadioSignalNoise *ScalarRadioBackgroundNoise::computeNoise(const IRadioSignalListening *listening) const
{
    const ScalarRadioSignalListening *scalarListening = check_and_cast<const ScalarRadioSignalListening *>(listening);
    simtime_t startTime = listening->getStartTime();
    simtime_t endTime = listening->getEndTime();
    std::map<simtime_t, double> *powerChanges = new std::map<simtime_t, double>();
    powerChanges->insert(std::pair<simtime_t, double>(startTime, power));
    powerChanges->insert(std::pair<simtime_t, double>(endTime, -power));
    return new ScalarRadioSignalNoise(startTime, endTime, powerChanges, scalarListening->getCarrierFrequency(), scalarListening->getBandwidth());
}

const IRadioSignalNoise *ScalarRadioBackgroundNoise::computeNoise(const IRadioSignalReception *reception) const
{
    const ScalarRadioSignalReception *scalarReception = check_and_cast<const ScalarRadioSignalReception *>(reception);
    simtime_t startTime = reception->getStartTime();
    simtime_t endTime = reception->getEndTime();
    std::map<simtime_t, double> *powerChanges = new std::map<simtime_t, double>();
    powerChanges->insert(std::pair<simtime_t, double>(startTime, power));
    powerChanges->insert(std::pair<simtime_t, double>(endTime, -power));
    return new ScalarRadioSignalNoise(startTime, endTime, powerChanges, scalarReception->getCarrierFrequency(), scalarReception->getBandwidth());
}

void ScalarRadioSignalListeningDecision::printToStream(std::ostream &stream) const
{
    RadioSignalListeningDecision::printToStream(stream);
    stream << ", maximum power = " << powerMaximum;
}

void ScalarRadioSignalReceptionDecision::printToStream(std::ostream &stream) const
{
    RadioSignalReceptionDecision::printToStream(stream);
    stream << ", SNR minimum = " << snrMinimum;
}

bool ScalarSNRRadioDecider::isReceptionPossible(const IRadioSignalReception *reception) const
{
    const ScalarRadioSignalReception *scalarReception = check_and_cast<const ScalarRadioSignalReception *>(reception);
    return scalarReception->getPower() >= sensitivity;
}

const IRadioSignalNoise *ScalarSNRRadioDecider::computeNoise(const std::vector<const IRadioSignalReception *> *receptions, const IRadioSignalNoise *backgroundNoise) const
{
    double carrierFrequency = -1;
    double bandwidth = -1;
    simtime_t noiseStartTime = SimTime::getMaxTime();
    simtime_t noiseEndTime = 0;
    std::map<simtime_t, double> *powerChanges = new std::map<simtime_t, double>();
    for (std::vector<const IRadioSignalReception *>::const_iterator it = receptions->begin(); it != receptions->end(); it++)
    {
        const ScalarRadioSignalReception *reception = check_and_cast<const ScalarRadioSignalReception *>(*it);
        if (carrierFrequency == -1)
            carrierFrequency = reception->getCarrierFrequency();
        else if (carrierFrequency != reception->getCarrierFrequency())
            throw cRuntimeError("Carrier frequency mismatch while computing noise");
        if (bandwidth == -1)
            bandwidth = reception->getBandwidth();
        else if (bandwidth != reception->getBandwidth())
            throw cRuntimeError("Bandwidth mismatch while computing noise");
        double power = reception->getPower();
        simtime_t startTime = reception->getStartTime();
        simtime_t endTime = reception->getEndTime();
        if (startTime < noiseStartTime)
            noiseStartTime = startTime;
        if (endTime > noiseEndTime)
            noiseEndTime = endTime;
        std::map<simtime_t, double>::iterator itStartTime = powerChanges->find(startTime);
        if (itStartTime != powerChanges->end())
            itStartTime->second += power;
        else
            powerChanges->insert(std::pair<simtime_t, double>(startTime, power));
        std::map<simtime_t, double>::iterator itEndTime = powerChanges->find(endTime);
        if (itEndTime != powerChanges->end())
            itEndTime->second -= power;
        else
            powerChanges->insert(std::pair<simtime_t, double>(endTime, -power));
    }
    if (backgroundNoise)
    {
        const ScalarRadioSignalNoise *scalarBackgroundNoise = check_and_cast<const ScalarRadioSignalNoise *>(backgroundNoise);
        if (carrierFrequency == -1)
            carrierFrequency = scalarBackgroundNoise->getCarrierFrequency();
        else if (carrierFrequency != scalarBackgroundNoise->getCarrierFrequency())
            throw cRuntimeError("Carrier frequency mismatch while computing noise");
        if (bandwidth == -1)
            bandwidth = scalarBackgroundNoise->getBandwidth();
        else if (bandwidth != scalarBackgroundNoise->getBandwidth())
            throw cRuntimeError("Bandwidth mismatch while computing noise");
        const std::map<simtime_t, double> *backgroundNoisePowerChanges = check_and_cast<const ScalarRadioSignalNoise *>(backgroundNoise)->getPowerChanges();
        for (std::map<simtime_t, double>::const_iterator it = backgroundNoisePowerChanges->begin(); it != backgroundNoisePowerChanges->end(); it++)
        {
            std::map<simtime_t, double>::iterator jt = powerChanges->find(it->first);
            if (jt != powerChanges->end())
                jt->second += it->second;
            else
                powerChanges->insert(std::pair<simtime_t, double>(it->first, it->second));
        }
    }
    return new ScalarRadioSignalNoise(noiseStartTime, noiseEndTime, powerChanges, carrierFrequency, bandwidth);
}

const IRadioSignalListeningDecision *ScalarSNRRadioDecider::computeListeningDecision(const IRadioSignalListening *listening, const std::vector<const IRadioSignalReception *> *overlappingReceptions, const IRadioSignalNoise *backgroundNoise) const
{
    const ScalarRadioSignalNoise *scalarNoise = check_and_cast<const ScalarRadioSignalNoise *>(computeNoise(overlappingReceptions, backgroundNoise));
    double maximumPower = scalarNoise->computeMaximumPower(listening->getStartTime(), listening->getEndTime());
    // TODO: KLUDGE: what is this / 10?
    return new ScalarRadioSignalListeningDecision(listening, maximumPower > sensitivity, maximumPower);
//    return new ScalarRadioSignalListeningDecision(listening, maximumPower > (sensitivity / 10), maximumPower);
}

const IRadioSignalReceptionDecision *ScalarSNRRadioDecider::computeReceptionDecision(const IRadioSignalListening *listening, const IRadioSignalReception *reception, const std::vector<const IRadioSignalReception *> *overlappingReceptions, const IRadioSignalNoise *backgroundNoise) const
{
    const ScalarRadioSignalListening *scalarListening = check_and_cast<const ScalarRadioSignalListening *>(listening);
    const ScalarRadioSignalReception *scalarReception = check_and_cast<const ScalarRadioSignalReception *>(reception);
    if (scalarListening->getCarrierFrequency() == scalarReception->getCarrierFrequency() && scalarListening->getBandwidth() == scalarReception->getBandwidth())
    {
        const IRadioSignalNoise *noise = computeNoise(overlappingReceptions, backgroundNoise);
        double snrMinimum = computeSNRMinimum(reception, noise);
        bool isReceptionPossible = computeIsReceptionPossible(reception, overlappingReceptions);
        bool isReceptionSuccessful = isReceptionPossible && snrMinimum > snrThreshold;
        return new ScalarRadioSignalReceptionDecision(reception, isReceptionPossible, isReceptionSuccessful, snrMinimum);
    }
    else if ((scalarListening->getCarrierFrequency() + scalarListening->getBandwidth() / 2 < scalarReception->getCarrierFrequency() - scalarReception->getBandwidth() / 2) ||
             (scalarListening->getCarrierFrequency() - scalarListening->getBandwidth() / 2 > scalarReception->getCarrierFrequency() + scalarReception->getBandwidth() / 2))
    {
        return new ScalarRadioSignalReceptionDecision(reception, false, false, 0);
    }
    else
        throw cRuntimeError("Overlapping carrier frequency and bandwidth is not supported");
}

double ScalarSNRRadioDecider::computeSNRMinimum(const IRadioSignalReception *reception, const IRadioSignalNoise *noise) const
{
    const ScalarRadioSignalNoise *scalarNoise = check_and_cast<const ScalarRadioSignalNoise *>(noise);
    const ScalarRadioSignalReception *scalarReception = check_and_cast<const ScalarRadioSignalReception *>(reception);
    return scalarReception->getPower() / scalarNoise->computeMaximumPower(reception->getStartTime(), reception->getEndTime());
}

void ScalarRadioSignalModulator::initialize(int stage)
{
    if (stage == INITSTAGE_LOCAL)
    {
        bitrate = par("bitrate");
        headerBitLength = par("headerBitLength");
        power = par("power");
        carrierFrequency = par("carrierFrequency");
        bandwidth = par("bandwidth");
    }
}

const IRadioSignalTransmission *ScalarRadioSignalModulator::createTransmission(const IRadio *radio, const cPacket *packet, simtime_t startTime) const
{
    simtime_t duration = (packet->getBitLength() + headerBitLength) / bitrate;
    simtime_t endTime = startTime + duration;
    IMobility *mobility = radio->getTransmitterAntenna()->getMobility();
    Coord startPosition = mobility->getPosition(startTime);
    Coord endPosition = mobility->getPosition(endTime);
    return new ScalarRadioSignalTransmission(radio, startTime, endTime, startPosition, endPosition, bitrate, power, carrierFrequency, bandwidth);
}

const IRadioSignalListening *ScalarRadioSignalModulator::createListening(const IRadio *radio, simtime_t startTime, simtime_t endTime, Coord startPosition, Coord endPosition) const
{
    // TODO:
    return new ScalarRadioSignalListening(radio, startTime, endTime, startPosition, endPosition, carrierFrequency, bandwidth);
}