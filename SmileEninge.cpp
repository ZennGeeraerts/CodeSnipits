void DirectX11RendererAPI::Initialize()
	{
		m_pDirectX11Context = static_cast<DirectX11Context*>(SmileGame::GetInstance().GetWindow().GetRenderingContext());
		SM_ASSERT(m_pDirectX11Context, "DirectX11RendererAPI > RenderingContext is not a DirectX11Context");
	}

	void DirectX11RendererAPI::ResizeWindow(uint32_t x, uint32_t y, uint32_t width, uint32_t height)
	{
		if (!m_pDirectX11Context->m_pSwapChain || !m_pDirectX11Context->m_pDevice || !m_pDirectX11Context->m_pDeviceContext)
			return;

		m_pDirectX11Context->m_pDeviceContext->OMSetRenderTargets(0, 0, 0);

		D3D11_TEXTURE2D_DESC depthStencilDesc{};
		m_pDirectX11Context->m_pDepthStencilBuffer->GetDesc(&depthStencilDesc);
		depthStencilDesc.Width = width;
		depthStencilDesc.Height = height;

		D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc{};
		m_pDirectX11Context->m_pDepthStencilView->GetDesc(&depthStencilViewDesc);

		SAFE_RELEASE(m_pDirectX11Context->m_pCurrentRenderTarget);
		SAFE_RELEASE(m_pDirectX11Context->m_pRenderTargetBuffer);
		SAFE_RELEASE(m_pDirectX11Context->m_pDepthStencilView);
		SAFE_RELEASE(m_pDirectX11Context->m_pDepthStencilBuffer);

		HRESULT result = m_pDirectX11Context->m_pSwapChain->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, 0);
		if (FAILED(result))
		{
			SM_LOG_ERROR("DirectX11RendererAPI::ResizeWindow > Failed to resize buffers");
			return;
		}

		// Depth stencil
		result = m_pDirectX11Context->m_pDevice->CreateTexture2D(&depthStencilDesc, 0, &m_pDirectX11Context->m_pDepthStencilBuffer);
		if (FAILED(result))
		{
			SM_LOG_ERROR("DirectX11RendererAPI::ResizeWindow > Failed to create depth stencil buffer");
			return;
		}

		result = m_pDirectX11Context->m_pDevice->CreateDepthStencilView(m_pDirectX11Context->m_pDepthStencilBuffer, &depthStencilViewDesc, &m_pDirectX11Context->m_pDepthStencilView);
		if (FAILED(result))
		{
			SM_LOG_ERROR("DirectX11RendererAPI::ResizeWindow > Failed to create depth stencil view");
			return;
		}

		// Render target
		result = m_pDirectX11Context->m_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&m_pDirectX11Context->m_pRenderTargetBuffer));
		if (FAILED(result))
		{
			SM_LOG_ERROR("DirectX11RendererAPI::ResizeWindow > Failed to get buffer from swap chain");
			return;
		}

		result = m_pDirectX11Context->m_pDevice->CreateRenderTargetView(m_pDirectX11Context->m_pRenderTargetBuffer, 0, &m_pDirectX11Context->m_pCurrentRenderTarget);
		if (FAILED(result))
		{
			SM_LOG_ERROR("DirectX11RendererAPI::ResizeWindow > Failed to create render target view");
			return;
		}

		m_pDirectX11Context->m_pDeviceContext->OMSetRenderTargets(1, &m_pDirectX11Context->m_pCurrentRenderTarget, m_pDirectX11Context->m_pDepthStencilView);
		
		D3D11_VIEWPORT viewPort{};
		viewPort.Width = static_cast<FLOAT>(width);
		viewPort.Height = static_cast<FLOAT>(height);
		viewPort.TopLeftX = static_cast<FLOAT>(x);
		viewPort.TopLeftY = static_cast<FLOAT>(y);
		viewPort.MinDepth = 0.0f;
		viewPort.MaxDepth = 1.0;

		m_pDirectX11Context->m_pDeviceContext->RSSetViewports(1, &viewPort);
	}

	void DirectX11RendererAPI::SetClearColor(const DirectX::XMFLOAT4& color)
	{
		m_ClearColor = color;
	}

	void DirectX11RendererAPI::Clear()
	{
		m_pDirectX11Context->m_pDeviceContext->OMSetRenderTargets(1, &m_pDirectX11Context->m_pCurrentRenderTarget, m_pDirectX11Context->m_pDepthStencilView);

		const float* pClearColor = reinterpret_cast<const float*>(&m_ClearColor);
		m_pDirectX11Context->m_pDeviceContext->ClearRenderTargetView(m_pDirectX11Context->m_pCurrentRenderTarget, pClearColor);
		m_pDirectX11Context->m_pDeviceContext->ClearDepthStencilView(m_pDirectX11Context->m_pDepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.f, 0);
	}

	void DirectX11RendererAPI::DrawIndexed(int32_t indexCount, const Ref<Shader>& pShader)
	{
		auto pDirectX11Shader = static_cast<DirectX11Shader*>(pShader.get());
		SM_ASSERT(pDirectX11Shader, "DirectX11RendererAPI::DrawIndexed > Shader is not a DirectX11Shader");

		m_pDirectX11Context->m_pDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		auto pTechnique = pDirectX11Shader->GetTechnique();
		D3DX11_TECHNIQUE_DESC techDesc{};
		pTechnique->GetDesc(&techDesc);
		for (UINT p{}; p < techDesc.Passes; ++p)
		{
			pTechnique->GetPassByIndex(p)->Apply(0, m_pDirectX11Context->m_pDeviceContext);
			m_pDirectX11Context->m_pDeviceContext->DrawIndexed(indexCount, 0, 0);
		}
	}
