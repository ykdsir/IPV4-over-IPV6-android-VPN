package com.test.a4over6_vpn;

import android.content.Intent;
import android.os.Environment;
import android.os.Handler;
import android.os.Message;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.widget.EditText;
import android.widget.TextView;
import android.widget.Button;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.net.VpnService;
import android.net.wifi.WifiInfo;
import android.net.wifi.WifiManager;
import android.content.Context;
import android.widget.TextView;
import android.widget.Toast;
import android.util.Log;

import java.io.BufferedOutputStream;
import java.io.BufferedInputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.net.InetAddress;
import java.net.Inet6Address;
import java.net.NetworkInterface;
import java.net.SocketException;
import java.util.Enumeration;

public class MainActivity extends AppCompatActivity {
    private static String[] info;
    private static String ipv4Addr;
    private static String router;
    private static String dns1;
    private static String dns2;
    private static String dns3;
    private static String sockfd;
    //TODO /* 界面修改，
      /* 上传、下载速度
             * 上传总流量和包数
             * 下载总流量和包数
             */
    private static String uploadSpeed;
    private static String downloadSpeed;
    private static String uploadTotalPkg;
    private static String downloadTotalPkg;
    private static String uploadTotalLength;
    private static String downloadTotalLength;

    private boolean IPflag =  true;

    private static final int MSG_UPDATEUI = 0;
    private static final int MSG_CONNECT  = 1;
    private static final int MSG_STATUS   = 2;
    private static final int MSG_DISCONNECT   = 3;

    File infoTunnel;
    FileInputStream infoInputStream;
    BufferedInputStream infoBufferStream;
    // Used to load the 'native-lib' library on application startup.
    static {
        System.loadLibrary("native-lib");
    }
    private Handler mHandler = new Handler() {
        public void handleMessage (Message msg) { // in main(UI) thread
            TextView editText = (TextView) findViewById(R.id.editText);
            TextView uploadspeed = (TextView)findViewById(R.id.UploadSpeed);
            TextView downloadspeed = (TextView)findViewById(R.id.DownloadSpeed);
            TextView uploadLength = (TextView)findViewById(R.id.UploadTotalLength);
            TextView downloadLength = (TextView)findViewById(R.id.DownloadTotalLength);
            TextView uploadPkg = (TextView)findViewById(R.id.UploadPkg);
            TextView downloadPkg = (TextView)findViewById(R.id.DownloadPkg);
            switch (msg.what) {
                case MSG_UPDATEUI:
                    Log.d("wjf", "MSG_UPDATEUI");
                    //Done 根据管道数据更新上传速度等信息
                    String update = (String)msg.obj;
                    Log.d("wjf", update);
                    info = update.split("");
                    if (info.length == 6){
                        uploadSpeed = info[0];
                        downloadSpeed = info[1];
                        uploadTotalLength = info[2];
                        uploadTotalPkg = info[3];
                        downloadTotalLength = info[4];
                        downloadTotalPkg = info[5];
                    }
                    //TODO 根据获得的上传、下载速度信息更新UI 更好看的界面  https://github.com/lecho/hellocharts-android
                    uploadspeed.setText(uploadSpeed);
                    downloadspeed.setText(downloadSpeed);
                    uploadLength.setText(uploadTotalLength);
                    downloadLength.setText(downloadTotalLength);
                    uploadPkg.setText(uploadTotalPkg);
                    downloadPkg.setText(downloadTotalPkg);
                    break;
                case MSG_STATUS:
                    String status = (String)msg.obj;
                    Log.d("wjf", status);
                    editText.setText(status + "\n");
                    break;
                case MSG_CONNECT:
                    editText.setText("");
                    String temp = (String)msg.obj;
                    Log.d("wjf", "MSG_CONNECT " + temp);
                    info = temp.split(" ");
                    Log.d("wjf", "info.split length " + info.length);
                    if (info.length == 6) {
                        ipv4Addr = info[0];
                        Log.d("wjf", "ipv4Addr.split length " + ipv4Addr.split("\\.").length);
                        if (ipv4Addr.split("\\.").length == 4) {
                            router   = info[1];
                            dns1     = info[2];
                            dns2     = info[3];
                            dns3     = info[4];
                            sockfd   = info[5];
                            editText.append(ipv4Addr + "\n");
                            editText.append(router + "\n");
                            editText.append(dns1 + "\n");
                            editText.append(dns2 + "\n");
                            editText.append(dns3 + "\n");
                            editText.append(info[5] + "\n");
                            Log.d("wjf", "Will start VPN now");
                            startVPNService();
                        } else {
                            Log.d("wjf", temp);
                        }
                    } else {
                        Log.e("wjf", "Wrong info from server.");
                    }
                    break;
                case MSG_DISCONNECT:
                    editText.setText((String)msg.obj);
                    break;
                default:
                    break;
            }
        }
    };
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        // Example of a call to a native method
        //Done 检查网络状态，获取IPV6地址
        checkNetStatus();
        //TODO 开启后台线程，调用startBackground()
        final Button connectButton = (Button)findViewById(R.id.ConnectButton);
        connectButton.setOnClickListener(new View.OnClickListener(){
            public void onClick(View view)
            {
                Toast.makeText(MainActivity.this,"kai qi background",Toast.LENGTH_LONG).show();
                Runnable background = new Runnable(){
                    public void run()
                    {
                        Log.d("background","start background thread");
                        String temp = startBackground();
                        Log.d("background","return" + temp);
                        Message message = new Message();
                        message.what = MSG_STATUS;
                        message.obj = temp;
                        mHandler.sendMessage(message);
                    }
                };
                Thread backgroundthr = new Thread(background);
                backgroundthr.start();
                //---------------------------------------------------------------------------------------------------------------------------------------------
                Runnable readIpPipe = new Runnable() {
                    @Override
                    public void run() {
                        File ipTunnel = createIpPipe();
                        createInfoPipe();
                        try {
                            byte[] buffer = new byte[1024];
                            FileInputStream fileInputStream = new FileInputStream(ipTunnel);
                            BufferedInputStream in = new BufferedInputStream(fileInputStream);
                            Log.d("wjf", "Buffered input stream opened");
                            int len = 0;
                            try{
                                Log.d("wjf","before read ip tunnel");
                                while(!IsIPTunnelChanged()) {

                                }
                                while(IPflag){
                                    len = in.read(buffer);
                                    Log.d("wjf","read ip tunnel");
                                    if(len > 0)
                                    {
                                        Message message = new Message();
                                        String ret = new String(buffer);
                                        ret = ret.substring(0, len);
                                        info = ret.split(" ");
                                        message.obj = ret;
                                        //TODO 通过handler更改UI
                                        if (info.length == 6) {
                                            IPflag = false;
                                            message.what = MSG_CONNECT;
                                        } else {
                                            message.what = MSG_STATUS;
//                                            Log.e("wjf", "Wrong "+ret);
                                        }
                                        mHandler.sendMessage(message);
                                    }
                                }
                                in.close();
                            }catch (IOException e){
                                e.printStackTrace();
                            }
                        } catch (FileNotFoundException e) {
                            e.printStackTrace();
                        }
                    }
                };
                Thread readIpPipeth = new Thread(readIpPipe);
                readIpPipeth.start();
            }
        });
        //TODO 开启前台计时器刷新界面

        //TODO 创建流量信息管道 info_pipe ，开启VPN服务，将安卓虚接口写入IP信息管道
//        out.write(arr, 0, arr.length)
//        out.flush();
//        out.close();
        final Button disConnectButton = (Button)findViewById(R.id.DisConnectButton);
        disConnectButton.setOnClickListener(new View.OnClickListener() {
            public void onClick(View view) {
                IPflag = true;
                Message message = new Message();
                message.what = MSG_DISCONNECT;
                message.obj = "disconnect";
                mHandler.sendMessage(message);
                disConnect();
            }
        });
    }

    private boolean isNetConnected() {
        ConnectivityManager connectivityManager =
                (ConnectivityManager) getSystemService(Context.CONNECTIVITY_SERVICE);
        if (connectivityManager != null) {
            NetworkInfo networkInfo = connectivityManager.getActiveNetworkInfo();
            if (networkInfo != null) {
                if (networkInfo.isConnected()) {
                    return true;
                }
            }
        }
        return false;
    }

    // check if wifi connected
    private boolean isWifiConnected() {
        ConnectivityManager connectivityManager = (ConnectivityManager)
                getSystemService(Context.CONNECTIVITY_SERVICE);

        if (connectivityManager != null) {
            NetworkInfo networkInfo = connectivityManager.getActiveNetworkInfo();
            if (networkInfo != null && networkInfo.getType() == ConnectivityManager.TYPE_WIFI) {
                return true;
            }
        }
        return false;
    }

    private void checkNetStatus() {
        TextView statusTextView = (TextView) findViewById(R.id.NetworkStatusTextView);
        if (!isNetConnected()) {
            Toast.makeText(MainActivity.this, "You are not connected to network", Toast.LENGTH_SHORT).show();
            statusTextView.setText("You are not connected to network" + "\n");
            return;
        }
        /*if (!isWifiConnected()) {
            Toast.makeText(MainActivity.this, "You are not connected by WiFi", Toast.LENGTH_SHORT).show();
            statusTextView.setText("You are not connected by WiFi\n");
            return;
        }*/
        statusTextView.setText("You are connected by WiFi\n");

        String macAddr = getMacAddress();
        TextView macTextView = (TextView) findViewById(R.id.MacTextView);
        macTextView.setText(macAddr);
        String ipv6Addr = getIPv6Address();
        TextView textView = (TextView)findViewById(R.id.IPv6AddressTextView);
        textView.setText(ipv6Addr);
        Log.d("wjf", ipv6Addr);
    }

    private String getMacAddress() {
        WifiManager wifiManager = (WifiManager) getApplicationContext().getSystemService(WIFI_SERVICE);
        WifiInfo wifiInfo = wifiManager.getConnectionInfo();
        return wifiInfo.getMacAddress();

    }

    private String getIPv6Address()
    {
        try{
            final Enumeration<NetworkInterface> e = NetworkInterface.getNetworkInterfaces();
            while (e.hasMoreElements())
            {
                final NetworkInterface networkInterface = e.nextElement();
                for(Enumeration<InetAddress>enumAddress = networkInterface.getInetAddresses();enumAddress.hasMoreElements();)
                {
                    InetAddress inetAddress = enumAddress.nextElement();
                    //TODO 检查原来的函数逻辑
//                    if(inetAddress instanceof Inet6Address
//                            && inetAddress.isSiteLocalAddress()
//                            && !inetAddress.isLoopbackAddress()
//                            && !isReservedAddr(inetAddress))
//                    {
//                        String ipAddr = inetAddress.getHostAddress();
//                        String ipAddripAddr = null;
//                        int index = ipAddr.indexOf('%');
//                        if (index > 0) {
//                            ipAddripAddr = ipAddr.substring(0, index);
//                        }
//                        return ipAddr;
//                    }
                    if (!inetAddress.isLoopbackAddress() && !inetAddress.isLinkLocalAddress()) {
                        return inetAddress.getHostAddress();
                    }
                }
            }
        }catch (SocketException e)
        {
            Log.wtf("WIFI_IP", "Unable to NetworkInterface.getNetworkInterfaces()");
        }
        return "";
    }
    private static boolean isReservedAddr(InetAddress inetAddr) {
        if (inetAddr.isAnyLocalAddress() || inetAddr.isLinkLocalAddress()
                || inetAddr.isLoopbackAddress()) {
            return true;
        }

        return false;
    }
    private boolean startVPNService(){
        Log.d("wjf","start");
        Intent intent = VpnService.prepare(this);
        if(intent != null)
        {
            startActivityForResult(intent , 0);
        }
        else
        {
            Log.d("wjf","no remind");
            onActivityResult(0,RESULT_OK,null);
        }

        return true;
    }
    protected void onActivityResult(int request,int result,Intent data)
    {
        if(result == RESULT_OK)
        {
            Log.d("wjf","result_ok");
            Intent intent = new Intent(MainActivity.this,MyVPNService.class);
            intent.putExtra("ipv4Addr", ipv4Addr);
            intent.putExtra("router", router);
            intent.putExtra("dns1", dns1);
            intent.putExtra("dns2", dns2);
            intent.putExtra("dns3", dns3);
            intent.putExtra("sockfd", sockfd);
            Log.d("wjf","startService()");
            startService(intent);
        }
    }

    private File createIpPipe(){
        String extDir = getApplicationInfo().dataDir;
        Log.d("wjf",extDir.toString());
        File ipTunnel = new File(extDir,"ip_pipe");
        if(ipTunnel.exists())
        {
            if(ipTunnel.isDirectory())
            {
                ipTunnel.delete();
                try {
                    ipTunnel.createNewFile();
                }catch (IOException e){
                    Log.d("wjf","IOEexception");
                }
            }
        }
        else
        {
            try {
                ipTunnel.createNewFile();
            }catch (IOException e){
                Log.d("wjf","IOEexception");
            }
        }
        return ipTunnel;
    }

    private void createInfoPipe(){
        String extDir = getApplicationInfo().dataDir;
        Log.d("wjf",extDir.toString());
        infoTunnel = new File(extDir,"info_pipe");
        if(infoTunnel.exists())
        {
            if(infoTunnel.isDirectory())
            {
                infoTunnel.delete();
                try {
                    infoTunnel.createNewFile();
                }catch (IOException e){
                    Log.d("wjf","IOEexception");
                }
            }
        }
        else
        {
            try {
                infoTunnel.createNewFile();
            }catch (IOException e){
                Log.d("wjf","IOEexception");
            }
        }

//        try{
//            infoInputStream = new FileInputStream(infoTunnel);
//            infoBufferStream = new BufferedInputStream(infoInputStream);
//        }catch (FileNotFoundException e){
//            e.printStackTrace();
//        }
    }

    private String readInfoPipe(){
        byte[] buffer = new byte[1024];
        int len ;
        try {
            infoInputStream = new FileInputStream(infoTunnel);
            infoBufferStream = new BufferedInputStream(infoInputStream);
            len = infoBufferStream.read(buffer);
            if (len > 0) {
                Message message = new Message();
                String ret = new String(buffer);
                ret = ret.substring(0, len);
                info = ret.split(" ");
                message.obj = ret;
                if (info.length == 6) {
                    message.what = MSG_UPDATEUI;
                } else {
                    message.what = MSG_STATUS;
                }
                mHandler.sendMessage(message);
            }
            infoInputStream.close();
            infoBufferStream.close();
        } catch (IOException e) {
            e.printStackTrace();
        }
        return "";
    }

    /**
     * A native method that is implemented by the 'native-lib' native library,
     * which is packaged with this application.
     */
    public native String startBackground();
    public native String test();
    public native boolean IsIPTunnelChanged();
    public native void disConnect();
}
