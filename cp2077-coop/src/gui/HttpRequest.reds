public struct HttpResponse {
    public var status: Uint16;
    public var body: String;
}

public class HttpRequest {
    private var url: String;
    private var status: Uint16;
    private var text: String;

    private static native func HttpRequest_HttpGet(url: String) -> HttpResponse
    private static native func HttpRequest_HttpPost(url: String, payload: String, mime: String) -> HttpResponse

    public func SetUrl(u: String) -> Void {
        url = u;
    }

    public func Send() -> Void {
        let res = HttpRequest_HttpGet(url);
        status = res.status;
        text = res.body;
        url = "";
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
}
