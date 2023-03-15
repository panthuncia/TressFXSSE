#pragma once
class NiStreamEx : public RE::NiStream {
public:
	MEMBER_FN_PREFIX(NiStream);
	DEFINE_MEMBER_FN(ctor, NiStream*, 0x00C59690);
	DEFINE_MEMBER_FN(dtor, void, 0x00C598F0);
};
