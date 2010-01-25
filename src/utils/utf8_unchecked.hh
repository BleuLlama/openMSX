// $Id$

// UTF8-CPP   http://utfcpp.sourceforge.net/
//   Slightly simplified (and reformatted) to fit openMSX coding style.

// Copyright 2006 Nemanja Trifunovic

/*
Permission is hereby granted, free of charge, to any person or organization
obtaining a copy of the software and accompanying documentation covered by
this license (the "Software") to use, reproduce, display, distribute,
execute, and transmit the Software, and to prepare derivative works of the
Software, and to permit third-parties to whom the Software is furnished to
do so, all subject to the following:

The copyright notices in the Software and this entire statement, including
the above license grant, this restriction and the following disclaimer,
must be included in all copies of the Software, in whole or in part, and
all derivative works of the Software, unless such copies or derivative
works are solely in the form of machine-executable object code generated by
a source language processor.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
DEALINGS IN THE SOFTWARE.
*/

#ifndef UTF8_UNCHECKED_HH
#define UTF8_UNCHECKED_HH

#include "utf8_core.hh"
#include <string>

namespace utf8 {
namespace unchecked {

template <typename octet_iterator>
octet_iterator append(uint32_t cp, octet_iterator result)
{
	if (cp < 0x80) {
		// one octet
		*result++ = cp;
	} else if (cp < 0x800) {
		// two octets
		*result++ = ((cp >>  6)       ) | 0xc0;
		*result++ = ((cp >>  0) & 0x3f) | 0x80;
	} else if (cp < 0x10000) {
		// three octets
		*result++ = ((cp >> 12)       ) | 0xe0;
		*result++ = ((cp >>  6) & 0x3f) | 0x80;
		*result++ = ((cp >>  0) & 0x3f) | 0x80;
	} else {
		// four octets
		*result++ = ((cp >> 18)       ) | 0xf0;
		*result++ = ((cp >> 12) & 0x3f) | 0x80;
		*result++ = ((cp >>  6) & 0x3f) | 0x80;
		*result++ = ((cp >>  0) & 0x3f) | 0x80;
	}
	return result;
}

template <typename octet_iterator>
uint32_t next(octet_iterator& it)
{
	uint32_t cp = *it;
	switch (utf8::internal::sequence_length(cp)) {
	case 1:
		break;
	case 2:
		++it;
		cp = ((cp << 6) & 0x7ff) + ((*it) & 0x3f);
		break;
	case 3:
		++it;
		cp = ((cp << 12) & 0xffff) + ((*it << 6) & 0xfff);
		++it;
		cp += (*it) & 0x3f;
		break;
	case 4:
		++it;
		cp = ((cp << 18) & 0x1fffff) + ((*it << 12) & 0x3ffff);
		++it;
		cp += (*it << 6) & 0xfff;
		++it;
		cp += (*it) & 0x3f;
		break;
	}
	++it;
	return cp;
}

template <typename octet_iterator>
uint32_t peek_next(octet_iterator it)
{
	return next(it);
}

template <typename octet_iterator>
uint32_t prior(octet_iterator& it)
{
	while (internal::is_trail(*(--it))) ;
	octet_iterator temp = it;
	return next(temp);
}

template <typename octet_iterator, typename distance_type>
void advance(octet_iterator& it, distance_type n)
{
	for (distance_type i = 0; i < n; ++i) {
		next(it);
	}
}

template <typename octet_iterator>
typename std::iterator_traits<octet_iterator>::difference_type
distance(octet_iterator first, octet_iterator last)
{
	typename std::iterator_traits<octet_iterator>::difference_type dist;
	for (dist = 0; first < last; ++dist) {
		next(first);
	}
	return dist;
}

template <typename u16bit_iterator, typename octet_iterator>
octet_iterator utf16to8(u16bit_iterator start, u16bit_iterator end,
                        octet_iterator result)
{
	while (start != end) {
		uint32_t cp = *start++;
		// Take care of surrogate pairs first
		if (internal::is_surrogate(cp)) {
			uint32_t trail_surrogate = *start++;
			cp = (cp << 10) + trail_surrogate + internal::SURROGATE_OFFSET;
		}
		result = append(cp, result);
	}
	return result;
}

template <typename u16bit_iterator, typename octet_iterator>
u16bit_iterator utf8to16(octet_iterator start, octet_iterator end,
                         u16bit_iterator result)
{
	while (start != end) {
		uint32_t cp = next(start);
		if (cp > 0xffff) {
			// make a surrogate pair
			*result++ = (cp >> 10)   + internal::LEAD_OFFSET;
			*result++ = (cp & 0x3ff) + internal::TRAIL_SURROGATE_MIN;
		} else {
			*result++ = cp;
		}
	}
	return result;
}

template <typename octet_iterator, typename u32bit_iterator>
octet_iterator utf32to8(u32bit_iterator start, u32bit_iterator end,
                        octet_iterator result)
{
	while (start != end) {
		result = append(*start++, result);
	}
	return result;
}

template <typename octet_iterator, typename u32bit_iterator>
u32bit_iterator utf8to32(octet_iterator start, octet_iterator end,
                         u32bit_iterator result)
{
	while (start < end) {
		*result++ = next(start);
	}
	return result;
}

// The iterator class
template <typename octet_iterator>
class iterator : public std::iterator<std::bidirectional_iterator_tag, uint32_t>
{
	octet_iterator it;
public:
	iterator() {};
	explicit iterator(const octet_iterator& octet_it)
		: it(octet_it) {}
	// the default "big three" are OK
	octet_iterator base() const { return it; }
	uint32_t operator*() const
	{
		octet_iterator temp = it;
		return next(temp);
	}
	bool operator==(const iterator& rhs) const
	{
		return it == rhs.it;
	}
	bool operator!=(const iterator& rhs) const
	{
		return !(operator==(rhs));
	}
	iterator& operator++()
	{
		std::advance(it, internal::sequence_length(*it));
		return *this;
	}
	iterator operator++(int)
	{
		iterator temp = *this;
		std::advance(it, internal::sequence_length(*it));
		return temp;
	}
	iterator& operator--()
	{
		prior(it);
		return *this;
	}
	iterator operator--(int)
	{
		iterator temp = *this;
		prior(it);
		return temp;
	}
};

// convenience functions
inline unsigned size(const std::string& utf8)
{
	return unsigned(utf8::unchecked::distance(utf8.begin(), utf8.end()));
}
inline std::string substr(const std::string& utf8, std::string::size_type first = 0,
                   std::string::size_type len = std::string::npos)
{
	std::string::const_iterator begin = utf8.begin();
	utf8::unchecked::advance(begin, first);
	std::string::const_iterator end;
	if (len != std::string::npos) {
		end = begin;
		while (len && (end != utf8.end())) {
			next(end); --len;
		}
	} else {
		end = utf8.end();
	}
	return std::string(begin, end);
}

} // namespace unchecked
} // namespace utf8

#endif
