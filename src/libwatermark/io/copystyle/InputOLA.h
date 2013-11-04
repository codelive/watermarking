#pragma once
#include <algorithm>

#include "Input.h"

template <typename data_type>
class InputOLA : public Input<data_type>
{
	public:
		InputOLA(const Parameters<data_type>& cfg):
			Input<data_type>(cfg)
		{
		}

		virtual typename Input<data_type>::size_type frameIncrement() final override
		{
			return Input<data_type>::conf.bufferSize / 2;
		}

		virtual void copy(typename std::vector<data_type>::const_iterator in,
						  typename std::vector<data_type>::iterator out,
						  typename CopyStyle<data_type>::size_type pos,
						  typename CopyStyle<data_type>::size_type big_vector_length) final override
		{
			if (frameIncrement() <= big_vector_length - pos)
			{
				std::copy_n(in + pos, frameIncrement(), out);
				std::fill_n(out + frameIncrement(), frameIncrement(), 0);
			}
			else
			{
				std::copy_n(in + pos, big_vector_length - pos, out);
				std::fill_n(out + big_vector_length - pos, frameIncrement() - (big_vector_length - pos), 0);

			}
		}

};
