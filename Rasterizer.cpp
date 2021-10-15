void Elite::Renderer::VertexTransformFunction(const std::vector<Mesh::Vertex_Input>& originalVertices, std::vector<Mesh::Vertex_Output>& transformedVertices, const Elite::FMatrix4& worldViewProjectionMatrix, const Elite::FMatrix4& worldTransform) const
{
	const size_t size{ originalVertices.size() };
	for (size_t i{}; i < size; ++i)
	{
		// Transform the vertex with the worldViewProjectionMatrix
		Elite::FPoint4 transformedVertex{ worldViewProjectionMatrix * Elite::FPoint4{ originalVertices[i].Position } };

		// Divide each component except w with the wValue
		transformedVertex.x /= transformedVertex.w;
		transformedVertex.y /= transformedVertex.w;
		transformedVertex.z /= transformedVertex.w;

		// Calculate the viewDirection of the vertex from the camera to the vertex
		const Elite::FVector3 viewDirection{ Elite::GetNormalized(originalVertices[i].Position - m_pCamera->GetOrigin()) };;
		transformedVertices.push_back(
    Mesh::Vertex_Output{ transformedVertex, originalVertices[i].Color, Elite::FVector3{ worldTransform * Elite::FVector4{ originalVertices[i].Normal } },
			 Elite::FVector3{ worldTransform * Elite::FVector4{ originalVertices[i].Tangent } }, originalVertices[i].UV, viewDirection }
       );
	}
}

Elite::RGBColor Elite::Renderer::PixelShading(const Mesh::Vertex_Output& vertexOutput)
{
	// Light
	const Elite::FVector3 lightDirection{ Elite::GetNormalized(Elite::FVector3{ .577f, -.577f, -.577f }) };
	const Elite::RGBColor lightColor{ 1.0f, 1.0f, 1.0f };
	const float lightIntensity{ 2.f };

	// Normal map
	const Elite::FVector3 binormal{ Elite::Cross(vertexOutput.Tangent, vertexOutput.Normal) };;
	const Elite::FMatrix3 tangentSpaceAxis{ vertexOutput.Tangent, binormal, vertexOutput.Normal };
	Elite::RGBColor sampledValue{ m_pNormalTexture->Sample(vertexOutput.UV) };
	sampledValue = 2.f * sampledValue - 1.f;
	const Elite::FVector3 bumpNormal{ Elite::FMatrix3{ tangentSpaceAxis } * Elite::FVector3{ sampledValue.r, sampledValue.g, sampledValue.b } }; // Normal of the normal map

	// Specular and gloss map
	const float shininess{ 25.f };
	const float specularIntensity{ m_pSpecularTexture->Sample(vertexOutput.UV).r };
	const Elite::FVector3 reflection{ Elite::GetNormalized(2 * lightIntensity * bumpNormal - lightDirection) };
	const float specularStrength{ m_pGlossinessTexture->Sample(vertexOutput.UV).r * shininess };
	float specular{ pow(Elite::Clamp(Elite::Dot(-reflection, vertexOutput.ViewDirection), 0.0f, 1.0f), specularStrength) };
	specular *= specularIntensity;

	// lambert cosine
	float observedArea{ Elite::Dot(-bumpNormal, lightDirection) };
	observedArea = Elite::Clamp(observedArea, 0.0f, 1.0f);

	// Calculate finalColor
	Elite::RGBColor finalColor{ lightColor * lightIntensity * (m_pDiffuseTexture->Sample(vertexOutput.UV) + Elite::RGBColor{ specular, specular, specular }) * observedArea };
	finalColor.MaxToOne();

	return finalColor;
}
