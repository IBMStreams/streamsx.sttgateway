package com.ibm.streamsx.sttgateway.test.httptestserver;

import java.io.IOException;
import java.io.PrintWriter;
import java.util.Enumeration;

import javax.servlet.ServletException;
import javax.servlet.http.HttpServlet;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;

@SuppressWarnings("serial")
public class IamAuthServlet extends HttpServlet {

	protected void doPost( HttpServletRequest request, HttpServletResponse response ) throws ServletException, IOException {
		
		System.out.println(request.getMethod() + " received from " + request.getRemoteHost() + ":" + request.getRemotePort() + " uri: " + request.getRequestURI());
		
		boolean hasRequiredContentType = false;
		boolean hasRequiredAccept = false;
		Enumeration<String> headers = request.getHeaderNames();
		while (headers.hasMoreElements()) {
			String head = headers.nextElement();
			String value = request.getHeader(head);
			System.out.println(head + ": " + value);
			if (head.equalsIgnoreCase("Content-Type"))
				if (value.equalsIgnoreCase("application/x-www-form-urlencoded"))
					hasRequiredContentType = true;
			if (head.equalsIgnoreCase("Accept"))
				if (value.equalsIgnoreCase("application/json"))
					hasRequiredAccept = true;
		}
		String grantType = null;
		String apiKey = null;
		String refreshToken = null;
		Enumeration<String> params = request.getParameterNames();
		while (params.hasMoreElements()) {
			String param = params.nextElement();
			String value = request.getParameter(param);
			System.out.println(param + ": " + value);
			if (param.equals("grant_type"))
				grantType = value;
			if (param.equals("apikey"))
				apiKey = value;
			if (param.equals("refresh_token"))
				refreshToken = value;
			
		}
		if (! hasRequiredAccept || ! hasRequiredContentType) {
			response.setContentType("text/html");
			response.setStatus(HttpServletResponse.SC_NOT_ACCEPTABLE);
			PrintWriter pw = response.getWriter();
			pw.print("header failure");
			return;
		}
		if (grantType == null) {
			response.setContentType("text/html");
			response.setStatus(HttpServletResponse.SC_NOT_ACCEPTABLE);
			PrintWriter pw = response.getWriter();
			pw.print("grantType == null");
			return;
		}

		if (grantType.equals("urn:ibm:params:oauth:grant-type:apikey")) {
			if (apiKey == null) {
				response.setContentType("text/html");
				response.setStatus(HttpServletResponse.SC_NOT_ACCEPTABLE);
				PrintWriter pw = response.getWriter();
				pw.print("apiKey == null");
				return;
			} else if (apiKey.equals("invalid")) {
				response.setContentType("text/html");
				response.setStatus(HttpServletResponse.SC_BAD_REQUEST);
				return;
			} else if (apiKey.equals("no_content")){
				response.setStatus(HttpServletResponse.SC_OK);
				return;
			} else if (apiKey.equals("valid")) {
				response.setStatus(HttpServletResponse.SC_OK);
				response.setContentType("application/json;charset=UTF-8");
				PrintWriter pw = response.getWriter();
				pw.println("{");
				pw.println("	\"access_token\":\"2YotnFZFEjr1zCsicMWpAA_access_0\",");
				pw.println("	\"refresh_token\":\"2YotnFZFEjr1zCsicMWpAA_refresh_0\",");
				pw.println("	\"scope\":\"ibm openid\",");
				pw.println("	\"expiration\":1577116347,");
				pw.println("	\"token_type\":\"Bearer\",");
				pw.println("	\"expires_in\":30");
				pw.println("}");
				return;
			} else {
				response.setContentType("text/html");
				response.setStatus(HttpServletResponse.SC_NOT_ACCEPTABLE);
				PrintWriter pw = response.getWriter();
				pw.print("unknown key: " + apiKey);
				return;
			}
		} else if (grantType.equals("refresh_token")) {
			if (refreshToken == null) {
				response.setContentType("text/html");
				response.setStatus(HttpServletResponse.SC_NOT_ACCEPTABLE);
				PrintWriter pw = response.getWriter();
				pw.print("refreshToken == null");
				return;
			} else if (refreshToken.startsWith("2YotnFZFEjr1zCsicMWpAA_refresh_")) {
				String numberString = refreshToken.substring(31);
				Integer myi = Integer.valueOf(numberString);
				int newi = myi.intValue() + 1;
				String newSequence = Integer.toString(newi);
				response.setStatus(HttpServletResponse.SC_OK);
				response.setContentType("application/json;charset=UTF-8");
				PrintWriter pw = response.getWriter();
				pw.println("{");
				pw.println("	\"access_token\":\"2YotnFZFEjr1zCsicMWpAA_access_" + newSequence + "\",");
				pw.println("	\"refresh_token\":\"2YotnFZFEjr1zCsicMWpAA_refresh_" + newSequence + "\",");
				pw.println("	\"scope\":\"ibm openid\",");
				pw.println("	\"expiration\":1577116347,");
				pw.println("	\"token_type\":\"Bearer\",");
				pw.println("	\"expires_in\":30");
				pw.println("}");
				return;
			} else {
				response.setContentType("text/html");
				response.setStatus(HttpServletResponse.SC_NOT_ACCEPTABLE);
				PrintWriter pw = response.getWriter();
				pw.print("unknown refresh_token: " + apiKey);
				return;
				
			}
		} else {
			response.setContentType("text/html");
			response.setStatus(HttpServletResponse.SC_NOT_ACCEPTABLE);
			PrintWriter pw = response.getWriter();
			pw.print("wrong grant_type: " + grantType);
			return;
		}
	}
}
