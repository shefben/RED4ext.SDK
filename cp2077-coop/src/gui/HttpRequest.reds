public struct HttpResponse {
    public var status: Uint16;
    public var body: String;
}

public struct HttpAsyncResult {
    public var token: Uint32;
    public var status: Uint16;
    public var body: String;
}

public class HttpRequest {
    private var url: String;
    private var status: Uint16;
    private var text: String;

    private static native func HttpRequest_HttpGet(url: String) -> HttpResponse
    private static native func HttpRequest_HttpPost(url: String, payload: String, mime: String) -> HttpResponse
    private static native func HttpRequest_HttpGetAsync(url: String) -> Uint32
    private static native func HttpRequest_PollAsync() -> HttpAsyncResult

    public func SetUrl(u: String) -> Void {
        url = u;
    }

    public func Send() -> Void {
        let res = HttpRequest_HttpGet(url);
        status = res.status;
        text = res.body;
        url = "";
    }

    public func SendAsync() -> Uint32 {
        let id = HttpRequest_HttpGetAsync(url);
        url = "";
        return id;
    }

    public func SendPost(payload: String, mime: String) -> Void {
        let res = HttpRequest_HttpPost(url, payload, mime);
        status = res.status;
        text = res.body;
        url = "";
    }

    public func GetStatusCode() -> Uint16 {
        return status;
    }

    public func GetBody() -> String {
        return text;
    }

    public func Clear() -> Void {
        url = "";
        status = 0u;
        text = "";
    }

    public static func PollAsync() -> HttpAsyncResult {
        return HttpRequest_PollAsync();
    }
}
