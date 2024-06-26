
#ifndef GAME_CLIENT_COMPONENTS_COUNTRYFLAGS_H
#define GAME_CLIENT_COMPONENTS_COUNTRYFLAGS_H
#include <base/vmath.h>
#include <base/tl/sorted_array.h>
#include <game/client/component.h>

class CCountryFlags : public CComponent, public CTextureUser
{
public:
	struct CCountryFlag
	{
		int m_CountryCode;
		char m_aCountryCodeString[8];
		int m_Texture;

		bool operator<(const CCountryFlag &Other) { return str_comp(m_aCountryCodeString, Other.m_aCountryCodeString) < 0; }
	};

	void OnInit();
	virtual void InitTextures();

	int Num() const;
	const CCountryFlag *GetByCountryCode(int CountryCode) const;
	const CCountryFlag *GetByIndex(int Index) const;
	void Render(int CountryCode, const vec4 *pColor, float x, float y, float w, float h);

private:
	enum
	{
		CODE_LB=-1,
		CODE_UB=999,
		CODE_RANGE=CODE_UB-CODE_LB+1,
	};
	sorted_array<CCountryFlag> m_aCountryFlags;
	int m_CodeIndexLUT[CODE_RANGE];

	void LoadCountryflagsIndexfile();
};
#endif
