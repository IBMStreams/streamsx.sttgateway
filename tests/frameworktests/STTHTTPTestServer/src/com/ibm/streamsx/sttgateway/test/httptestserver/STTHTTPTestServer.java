/**
 * 
 */
package com.ibm.streamsx.sttgateway.test.httptestserver;

import java.io.File;
import java.nio.file.Path;

import org.eclipse.jetty.http.HttpVersion;
import org.eclipse.jetty.security.ConstraintSecurityHandler;
import org.eclipse.jetty.server.Connector;
import org.eclipse.jetty.server.HttpConfiguration;
import org.eclipse.jetty.server.HttpConnectionFactory;
import org.eclipse.jetty.server.SecureRequestCustomizer;
import org.eclipse.jetty.server.Server;
import org.eclipse.jetty.server.ServerConnector;
import org.eclipse.jetty.server.SslConnectionFactory;
import org.eclipse.jetty.servlet.DefaultServlet;
import org.eclipse.jetty.servlet.ServletContextHandler;
import org.eclipse.jetty.util.resource.PathResource;
import org.eclipse.jetty.util.ssl.SslContextFactory;

/**
 * @author joergboe
 *
 */
public class STTHTTPTestServer {

	/**
	 * @param args
	 * @throws Exception 
	 */
	public static void main(String[] args) throws Exception {
		
		int httpPort = 8097;
		int httpsPort = 1443;
		if (args.length != 2) {
			System.out.println("HTTPTest server requires 2 arguments: httpPort httpsPort");
		} else {
			httpPort = Integer.parseInt(args[0]);
			httpsPort = Integer.parseInt(args[1]);
		}
		
		System.out.println("Start HTTP Test server listening on ports:");
		System.out.print(" - http  port: "); System.out.println(httpPort);
		System.out.print(" - https port: ");System.out.println(httpsPort);
		
		Server server = new Server();

		HttpConfiguration http_config = new HttpConfiguration();
		http_config.setSecureScheme("https");
		http_config.setSecurePort(httpsPort);
		http_config.setOutputBufferSize(32768);

		// HTTP connector
		ServerConnector http = new ServerConnector(server, new HttpConnectionFactory(http_config));
		//http.setHost("localhost");
		http.setPort(httpPort);
		http.setIdleTimeout(30000);

		// ssl context1
		SslContextFactory sslContextFactory = new SslContextFactory();
		sslContextFactory.setKeyStorePath("etc/keystore.jks");
		//sslContextFactory.setKeyStorePassword("changeit"); //store password is not required
		sslContextFactory.setKeyManagerPassword("changeit"); //key password is required
		sslContextFactory.setCertAlias("mykey");
		
		sslContextFactory.setRenegotiationAllowed(false);
		sslContextFactory.setExcludeProtocols("SSLv3");
		sslContextFactory.setIncludeProtocols("TLSv1.2", "TLSv1.1");
		
		System.out.println("********************************");
		System.out.println(sslContextFactory.dump());
		System.out.println("********************************");
		String[] exlist = sslContextFactory.getExcludeCipherSuites();
		System.out.println("Excluded CipherSuites:");
		for (int x=0; x<exlist.length; x++) {
			System.out.println(exlist[x]);
		}
		String[] exlist2 = sslContextFactory.getExcludeProtocols();
		for (int x=0; x<exlist2.length; x++) {
			System.out.println(exlist2[x]);
		}
		System.out.println("Included CipherSuites:");
		String[] exlist3 = sslContextFactory.getIncludeCipherSuites();
		for (int x=0; x<exlist3.length; x++) {
			System.out.println(exlist3[x]);
		}
		System.out.println("********************************");
		
		//must manipulate the excluded cipher specs to avoid that all specs are disables with streams java ssl engine
		String[] specs = {"^.*_(MD5|SHA|SHA1)$","^TLS_RSA_.*$","^.*_NULL_.*$","^.*_anon_.*$"};
		//String[] specs = {};
		sslContextFactory.setExcludeCipherSuites(specs);
		System.out.println("********************************");
		System.out.println(sslContextFactory.dump());
		System.out.println("********************************");
		
		// HTTPS Configuration
		// A new HttpConfiguration object is needed for the next connector and
		// you can pass the old one as an argument to effectively clone the
		// contents. On this HttpConfiguration object we add a
		// SecureRequestCustomizer which is how a new connector is able to
		// resolve the https connection before handing control over to the Jetty
		// Server.
		HttpConfiguration https_config = new HttpConfiguration(http_config);
		SecureRequestCustomizer src = new SecureRequestCustomizer();
		src.setStsMaxAge(2000);
		src.setStsIncludeSubDomains(true);
		https_config.addCustomizer(src);
		
		// HTTPS connector
		// We create a second ServerConnector, passing in the http configuration
		// we just made along with the previously created ssl context factory.
		// Next we set the port and a longer idle timeout.
		ServerConnector https = new ServerConnector(server,
				new SslConnectionFactory(sslContextFactory,HttpVersion.HTTP_1_1.asString()),
				new HttpConnectionFactory(https_config));
		https.setPort(httpsPort);
		https.setIdleTimeout(500000);
		server.setConnectors(new Connector[] { http, https });

		//Servlet context handler and context (static)
		Path webRootPath = new File("webapps/static-root/").toPath().toRealPath();
		ServletContextHandler context = new ServletContextHandler();
		context.setContextPath("/");
		context.setBaseResource(new PathResource(webRootPath));
		context.setWelcomeFiles(new String[] { "index.html" });
		//add servlets
		context.addServlet(HelloServlet.class, "/hello/*");
		context.addServlet(IamAuthServlet.class, "/access/*");
		context.addServlet(DefaultServlet.class,"/"); // always last, always on "/"

		ConstraintSecurityHandler security = new ConstraintSecurityHandler();

		security.setHandler(context);
		server.setHandler(security);

		server.start();
		server.dumpStdErr();
		server.join();
	}

}
