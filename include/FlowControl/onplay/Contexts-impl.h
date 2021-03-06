#pragma once

#include <assert.h>
#include "FlowControl/onplay/Contexts.h"
#include "FlowControl/FlowContext.h"

namespace FlowControl
{
	namespace onplay
	{
		namespace context
		{
			inline state::CardRef OnPlay::GetTarget() const {
				return manipulate_.GetSpecifiedTarget();
			}
		}
	}
}