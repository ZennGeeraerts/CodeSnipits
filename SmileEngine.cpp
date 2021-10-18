// Renderer API	
	void DirectX11RendererAPI::Initialize()
	{
		m_pDirectX11Context = static_cast<DirectX11Context*>(SmileGame::GetInstance().GetWindow().GetRenderingContext());
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



// Renderer
	void Renderer::BeginScene(const Camera& camera, const DirectX::XMFLOAT4X4& cameraTransform)
	{
		auto cameraTransformMat = DirectX::XMLoadFloat4x4(&cameraTransform);
		auto projectionMatrixMat = DirectX::XMLoadFloat4x4(&camera.GetProjectionMatrix());
		auto ViewMatrixMat = DirectX::XMMatrixInverse(nullptr, cameraTransformMat);
		auto viewProjectionMatrixMat = ViewMatrixMat * projectionMatrixMat;

		DirectX::XMStoreFloat4x4(&m_pSceneData->ViewProjectionMatrix, viewProjectionMatrixMat);
		DirectX::XMStoreFloat4x4(&m_pSceneData->ViewInverseMatrix, cameraTransformMat);
	}

	void Renderer::Submit(const Ref<VertexBuffer>& pVertexBuffer, const Ref<IndexBuffer>& pIndexBuffer, const Ref<Shader>& pShader, 
		const DirectX::XMFLOAT4X4& worldTransform)
	{
		pVertexBuffer->Bind();
		pIndexBuffer->Bind();
		pShader->Bind();

		pShader->UploadMat4("ViewProjection", m_pSceneData->ViewProjectionMatrix);
		pShader->UploadMat4("World", worldTransform);
		pShader->UploadMat4("ViewInverse", m_pSceneData->ViewInverseMatrix);
		RenderCommand::DrawIndexed(pIndexBuffer->GetCount(), pShader);
	}

	void Renderer::Submit(const MeshRendererComponent& meshRendererComponent, const DirectX::XMFLOAT4X4& worldTransform)
	{
		Submit(meshRendererComponent.pVertexBuffer, meshRendererComponent.pIndexBuffer, meshRendererComponent.pShader, worldTransform);
	}



// Windows Window
	WindowsWindow::WindowsWindow(const WindowSettings& settings)
	{
		Init(settings);
	}

	WindowsWindow::~WindowsWindow()
	{
		ShutDown();
	}

	void WindowsWindow::ShutDown()
	{
		DestroyWindow(m_WindowHandle);
		delete m_pContext;
	}

	void WindowsWindow::Init(const WindowSettings& settings)
	{
		m_Data.Title = settings.Title;
		m_Data.Height = settings.Height;
		m_Data.Width = settings.Width;

		m_Message = { 0 };

		SM_LOG_INFO("WindowsWindow::Init > Creating window: %s (%d, %d)", settings.Title.c_str(), settings.Width, settings.Height);

		// Create window class
		const wchar_t* className = L"SmileWindowClass";
		WNDCLASSEX windowClass;
		windowClass.cbSize = sizeof(WNDCLASSEX);
		windowClass.style = CS_HREDRAW | CS_VREDRAW;
		windowClass.cbClsExtra = 0;
		windowClass.cbWndExtra = 0;

		windowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
		windowClass.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);

		windowClass.hIcon = static_cast<HICON>(LoadImage(HINSTANCE(),
			MAKEINTRESOURCE(IDI_ICON1),
			IMAGE_ICON,
			256, 256,
			LR_DEFAULTCOLOR));

			/*LoadIcon(0, IDI_APPLICATION);*/
		windowClass.hIconSm = LoadIcon(0, IDI_APPLICATION);

		windowClass.lpszClassName = className;
		windowClass.lpszMenuName = nullptr;

		windowClass.hInstance = HINSTANCE();
		windowClass.lpfnWndProc = WindowsProcedureStatic;

		int success = RegisterClassEx(&windowClass);
		SM_ASSERT(success, "Could not register window class!");

		RECT windowRect{};
		windowRect.left = 0;
		windowRect.right = settings.Width + windowRect.left;
		windowRect.top = 0;
		windowRect.bottom = settings.Height + windowRect.top;
		AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX, FALSE);

		// Create and display the window
		auto windowTitle = std::wstring{ settings.Title.begin(), settings.Title.end() };
		m_WindowHandle = CreateWindow(className,
				windowTitle.c_str(),
				WS_OVERLAPPEDWINDOW | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX,
				CW_USEDEFAULT,
				CW_USEDEFAULT,
				windowRect.right - windowRect.left,
				windowRect.bottom - windowRect.top,
				nullptr,
				nullptr,
				HINSTANCE(),
				this);

		SM_ASSERT(m_WindowHandle, "WindowsWindow::Init > Could not create window!")

		// Init context
		m_pContext = new DirectX11Context{ this };
		m_pContext->Init();

		ShowWindow(m_WindowHandle, SW_SHOW);
		UpdateWindow(m_WindowHandle);
		SM_LOG_INFO("WindowsWindow::Init > Window '%s' created", settings.Title.c_str());

		SetVSync(true);
		m_bInitialized = true;
	}

	void WindowsWindow::OnUpdate()
	{
		if (!m_bInitialized)
			return;

		PollEvents();
		m_pContext->Present();
	}

	void WindowsWindow::PollEvents()
	{
		if (m_Message.message != WM_QUIT)
		{
			// If there are window messages then process them
			if (PeekMessage(&m_Message, 0, 0, 0, PM_REMOVE))
			{
				TranslateMessage(&m_Message);
				DispatchMessage(&m_Message);
			}
		}
	}


// DirectX 11 Vetex Buffer
	DirectX11VertexBuffer::DirectX11VertexBuffer(const VertexBufferData& vertexBufferData)
		: m_Layout{ vertexBufferData.BufferLayout }
	{
		m_pDirectX11Context = static_cast<DirectX11Context*>(SmileGame::GetInstance().GetWindow().GetRenderingContext());

		D3D11_BUFFER_DESC bd = {};
		bd.Usage = BufferUsageToDirectXType(vertexBufferData.Usage);
		bd.ByteWidth = vertexBufferData.BufferLayout.GetStride() * vertexBufferData.Count;
		bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		bd.CPUAccessFlags = 0;
		bd.MiscFlags = 0;

		D3D11_SUBRESOURCE_DATA initData = { 0 };
		initData.pSysMem = vertexBufferData.pVertices;

		HRESULT result = m_pDirectX11Context->GetDevice()->CreateBuffer(&bd, &initData, &m_pVertexBuffer);
		if (FAILED(result))
		{
			SM_LOG_ERROR("DirectX11VertexBuffer > Failed to create vertex buffer");
			return;
		}
	}

	DirectX11VertexBuffer::~DirectX11VertexBuffer()
	{
		SAFE_RELEASE(m_pVertexBuffer);
	}

	void DirectX11VertexBuffer::Bind() const
	{
		uint32_t offset{ 0 };
		uint32_t stride{ m_Layout.GetStride() };
		m_pDirectX11Context->GetDeviceContext()->IASetVertexBuffers(0, 1, &m_pVertexBuffer, &stride, &offset);
	}

	void DirectX11VertexBuffer::Unbind() const
	{
		m_pDirectX11Context->GetDeviceContext()->IASetVertexBuffers(0, 0, nullptr, nullptr, nullptr);
	}
