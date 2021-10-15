Elite::RGBColor BRDF::Lambert(const Elite::RGBColor& color, float reflectance)
{
	return Lambert(color, Elite::RGBColor{ reflectance, reflectance, reflectance });
}

Elite::RGBColor BRDF::Lambert(const Elite::RGBColor& color, const Elite::RGBColor& reflectance)
{
	return color * reflectance / float(E_PI);
}

Elite::RGBColor BRDF::Phong(float specularReflectance, int phongExponent, const Elite::FVector3& w0, const Elite::FVector3& w1, const Elite::FVector3 hitNormal)
{
	Elite::FVector3 reflect{ w0 - 2 * Elite::Dot(hitNormal, w0) * hitNormal };
	float cosa{ Elite::Dot(reflect, w1) };
	return Elite::RGBColor{ specularReflectance, specularReflectance, specularReflectance } * powf(cosa, phongExponent);
}

Elite::RGBColor BRDF::CookTorrance(const Elite::FVector3& w0, const Elite::FVector3& w1, const Elite::FVector3 hitNormal, float roughnessSqr, const Elite::RGBColor& baseReflectivity, Elite::RGBColor& fresnelResult)
{
	Elite::FVector3 halfVector{ Elite::GetNormalized((-w1 + w0) / Elite::SqrMagnitude(-w1 + w0)) };

	float normalDistribution{ TrowbridgeReitzGGX(hitNormal, halfVector, roughnessSqr) };
	fresnelResult = Schlick(halfVector, -w1, baseReflectivity);

	float k{};
	// direct lighting
	k = Elite::Square(roughnessSqr + 1) / 8;

	float geometry{ Smith(hitNormal, -w0, w1, k) };

	return (Elite::RGBColor{ normalDistribution, normalDistribution, normalDistribution } * fresnelResult * geometry)
		/ (4 * Elite::Dot(-w1, hitNormal) * Elite::Dot(w0, hitNormal));
}

float BRDF::TrowbridgeReitzGGX(const Elite::FVector3& hitNormal, const Elite::FVector3& halfVector, float roughnessSqr)
{
	return Elite::Square(roughnessSqr) 
		/ (float(E_PI) * Elite::Square(Elite::Square(Elite::Dot(hitNormal, halfVector)) * (Elite::Square(roughnessSqr) - 1) + 1));
}

Elite::RGBColor BRDF::Schlick(const Elite::FVector3& halfVector, const Elite::FVector3& w1, const Elite::RGBColor& baseReflectivity)
{
	return baseReflectivity + (Elite::RGBColor{ 1, 1, 1 } - baseReflectivity) * pow(1 - Elite::Dot(halfVector, w1), 5);
}

float BRDF::Smith(const Elite::FVector3& hitNormal, const Elite::FVector3& w0, const Elite::FVector3& w1, float k)
{
	return SchlickGGX(hitNormal, w1, k) * SchlickGGX(hitNormal, w0, k);
}

float BRDF::SchlickGGX(const Elite::FVector3& hitNormal, const Elite::FVector3& dir, float k)
{
	float nDot{ std::max(Elite::Dot(hitNormal, dir), 0.0f) };
	return nDot / (nDot * (1 - k) + k);
}
