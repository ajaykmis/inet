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

#ifndef __INET_IRADIOFRAME_H_
#define __INET_IRADIOFRAME_H_

#include "IPhysicalLayerFrame.h"
#include "IRadioSignal.h"
#include "IRadioSignalTransmission.h"

/**
 * This purely virtual interface provides an abstraction for different radio frames.
 */
class INET_API OldIRadioFrame : public IPhysicalLayerFrame
{
    public:
        virtual ~OldIRadioFrame() { }

        virtual IRadioSignal *getRadioSignal() = 0;
};

// TODO: merge with OldIRadioFrame
class INET_API IRadioFrame
{
    public:
        virtual ~IRadioFrame() {}

        virtual const IRadioSignalTransmission *getTransmission() const = 0;
};

#endif