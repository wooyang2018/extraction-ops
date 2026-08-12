[CmdletBinding(DefaultParameterSetName = 'Call')]
param(
    [Parameter(Mandatory = $true, ParameterSetName = 'Call')]
    [ValidateSet('list_toolsets', 'describe_toolset', 'call_tool')]
    [string]$Method,

    [Parameter(ParameterSetName = 'Call')]
    [string]$ParamsJson = '{}',

    [Parameter(ParameterSetName = 'Call')]
    [string]$ParamsPath,

    [string]$Uri = 'http://127.0.0.1:8000/mcp'
)

$ErrorActionPreference = 'Stop'

Add-Type -AssemblyName System.Net.Http

function Invoke-McpRequest {
    param(
        [Parameter(Mandatory = $true)]
        [System.Net.Http.HttpClient]$Client,

        [Parameter(Mandatory = $true)]
        [string]$RequestUri,

        [Parameter(Mandatory = $true)]
        [string]$Json
    )

    $content = [System.Net.Http.StringContent]::new(
        $Json,
        [System.Text.Encoding]::UTF8,
        'application/json')
    try {
        $response = $Client.PostAsync($RequestUri, $content).GetAwaiter().GetResult()
        $body = $response.Content.ReadAsStringAsync().GetAwaiter().GetResult()
        if (-not $response.IsSuccessStatusCode) {
            throw "MCP request failed with HTTP $([int]$response.StatusCode): $body"
        }

        return [PSCustomObject]@{
            Response = $response
            Body = $body
        }
    }
    finally {
        $content.Dispose()
    }
}

$client = [System.Net.Http.HttpClient]::new()
try {
    $client.DefaultRequestHeaders.Accept.ParseAdd('application/json')
    $client.DefaultRequestHeaders.Accept.ParseAdd('text/event-stream')

    $initialize = @{
        jsonrpc = '2.0'
        id = 1
        method = 'initialize'
        params = @{
            protocolVersion = '2025-06-18'
            capabilities = @{}
            clientInfo = @{
                name = 'ExtractionOpsAutomation'
                version = '1.0'
            }
        }
    } | ConvertTo-Json -Depth 10 -Compress

    $initializeResult = Invoke-McpRequest -Client $client -RequestUri $Uri -Json $initialize
    $sessionId = ($initializeResult.Response.Headers.GetValues('Mcp-Session-Id') | Select-Object -First 1)
    if ([string]::IsNullOrWhiteSpace($sessionId)) {
        throw 'The Unreal MCP server did not return an Mcp-Session-Id header.'
    }

    $client.DefaultRequestHeaders.Add('Mcp-Session-Id', $sessionId)

    $initialized = @{
        jsonrpc = '2.0'
        method = 'notifications/initialized'
        params = @{}
    } | ConvertTo-Json -Depth 5 -Compress
    [void](Invoke-McpRequest -Client $client -RequestUri $Uri -Json $initialized)

    if (-not [string]::IsNullOrWhiteSpace($ParamsPath)) {
        $ParamsJson = Get-Content -LiteralPath $ParamsPath -Raw
    }

    try {
        $params = $ParamsJson | ConvertFrom-Json
    }
    catch {
        throw "ParamsJson must be a JSON object: $($_.Exception.Message)"
    }

    $request = @{
        jsonrpc = '2.0'
        id = 2
        method = "tools/call"
        params = @{
            name = $Method
            arguments = $params
        }
    } | ConvertTo-Json -Depth 30 -Compress

    $result = Invoke-McpRequest -Client $client -RequestUri $Uri -Json $request
    $result.Body
}
finally {
    $client.Dispose()
}
