// ============================================================================
// HTTP Request System
// ============================================================================
// Handles HTTP requests for server browser and other web-based functionality

public class HttpRequest {
    private static var nextToken: Uint32 = 1u;
    private static var pendingRequests: array<Uint32>;
    
    private var url: String;
    private var method: String;
    private var data: String;
    private var headers: array<String>;
    private var timeout: Uint32;
    private var token: Uint32;
    
    public func new() {
        this.method = "GET";
        this.data = "";
        this.timeout = 30000u; // 30 seconds
        this.token = 0u;
    }
    
    public func SetUrl(urlString: String) -> Void {
        this.url = urlString;
    }
    
    public func SetMethod(methodString: String) -> Void {
        this.method = methodString;
    }
    
    public func SetData(dataString: String) -> Void {
        this.data = dataString;
    }
    
    public func SetTimeout(timeoutMs: Uint32) -> Void {
        this.timeout = timeoutMs;
    }
    
    public func AddHeader(header: String) -> Void {
        ArrayPush(this.headers, header);
    }
    
    public func SendAsync() -> Uint32 {
        if this.url == "" {
            LogChannel(n"ERROR", "HttpRequest: URL not set");
            return 0u;
        }
        
        this.token = nextToken;
        nextToken += 1u;
        
        // Add to pending requests
        ArrayPush(pendingRequests, this.token);
        
        LogChannel(n"DEBUG", "HttpRequest: Sending " + this.method + " to " + this.url);
        
        // Build headers string
        let headersString = "";
        for header in this.headers {
            headersString += header + "\n";
        }
        
        // Send request via native layer
        let success = Http_SendAsync(this.url, this.method, this.data);
        if !success {
            LogChannel(n"ERROR", "HttpRequest: Failed to send request");
            ArrayRemove(pendingRequests, this.token);
            return 0u;
        }
        
        return this.token;
    }
    
    public static func PollAsync() -> HttpAsyncResult {
        let result: HttpAsyncResult;
        result.token = 0u;
        result.status = 0u;
        result.body = "";
        
        // Poll native HTTP system for completed requests
        let response = Http_PollResponse(0u); // Poll any completed request
        if IsDefined(response) && response.token != 0u {
            result.token = response.token;
            result.status = response.status;
            result.body = response.body;
            
            // Remove from pending requests
            ArrayRemove(pendingRequests, response.token);
            
            LogChannel(n"DEBUG", "HttpRequest: Received response for token " + IntToString(response.token) + 
                      " with status " + IntToString(response.status));
        }
        
        return result;
    }
    
    public static func PollToken(token: Uint32) -> HttpAsyncResult {
        let result: HttpAsyncResult;
        result.token = 0u;
        result.status = 0u;
        result.body = "";
        
        if token == 0u {
            return result;
        }
        
        // Poll specific token
        let response = Http_PollResponse(token);
        if IsDefined(response) && response.token == token {
            result.token = response.token;
            result.status = response.status;
            result.body = response.body;
            
            // Remove from pending requests
            ArrayRemove(pendingRequests, token);
            
            LogChannel(n"DEBUG", "HttpRequest: Received response for specific token " + IntToString(token) + 
                      " with status " + IntToString(response.status));
        }
        
        return result;
    }
    
    public static func IsRequestPending(token: Uint32) -> Bool {
        if token == 0u {
            return false;
        }
        
        let count = ArraySize(pendingRequests);
        var i = 0;
        while i < count {
            if pendingRequests[i] == token {
                return true;
            }
            i += 1;
        }
        
        return false;
    }
    
    public static func GetPendingRequestCount() -> Int32 {
        return ArraySize(pendingRequests);
    }
    
    public static func CancelRequest(token: Uint32) -> Bool {
        if token == 0u {
            return false;
        }
        
        // Remove from pending requests
        let wasRemoved = ArrayRemove(pendingRequests, token);
        if wasRemoved {
            LogChannel(n"DEBUG", "HttpRequest: Cancelled request with token " + IntToString(token));
        }
        
        return wasRemoved;
    }
    
    public static func CancelAllRequests() -> Void {
        let cancelledCount = ArraySize(pendingRequests);
        ArrayClear(pendingRequests);
        
        if cancelledCount > 0 {
            LogChannel(n"DEBUG", "HttpRequest: Cancelled " + IntToString(cancelledCount) + " pending requests");
        }
    }
    
    // Convenience methods for common HTTP operations
    public static func Get(url: String) -> Uint32 {
        let request = new HttpRequest();
        request.SetUrl(url);
        request.SetMethod("GET");
        return request.SendAsync();
    }
    
    public static func Post(url: String, data: String) -> Uint32 {
        let request = new HttpRequest();
        request.SetUrl(url);
        request.SetMethod("POST");
        request.SetData(data);
        request.AddHeader("Content-Type: application/json");
        return request.SendAsync();
    }
    
    public static func Put(url: String, data: String) -> Uint32 {
        let request = new HttpRequest();
        request.SetUrl(url);
        request.SetMethod("PUT");
        request.SetData(data);
        request.AddHeader("Content-Type: application/json");
        return request.SendAsync();
    }
    
    public static func Delete(url: String) -> Uint32 {
        let request = new HttpRequest();
        request.SetUrl(url);
        request.SetMethod("DELETE");
        return request.SendAsync();
    }
}

// Result structure for async HTTP operations
public struct HttpAsyncResult {
    var token: Uint32;
    var status: Uint16;
    var body: String;
}

// Extended HTTP response with headers
public struct HttpDetailedResponse {
    var token: Uint32;
    var status: Uint16;
    var statusText: String;
    var body: String;
    var headers: array<String>;
    var contentType: String;
    var contentLength: Uint32;
}